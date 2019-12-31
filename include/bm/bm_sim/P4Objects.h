/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_P4OBJECTS_H_
#define BM_BM_SIM_P4OBJECTS_H_

#include <iosfwd>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <set>
#include <tuple>
#include <utility>  // for pair<>

#include "tables.h"
#include "headers.h"
#include "phv_forward.h"
#include "parser.h"
#include "deparser.h"
#include "pipeline.h"
#include "conditionals.h"
#include "checksums.h"
#include "control_flow.h"
#include "learning.h"
#include "meters.h"
#include "counters.h"
#include "stateful.h"
#include "ageing.h"
#include "field_lists.h"
#include "extern.h"
#include "enums.h"
#include "control_action.h"
#include "device_id.h"

// forward declaration of Json::Value
namespace Json {

class Value;

}  // namespace Json

namespace bm {

using ConfigOptionMap = std::unordered_map<std::string, std::string>;

class P4Objects {
 public:
  using header_field_pair = std::pair<std::string, std::string>;

  enum class ResourceType {
    MATCH_TABLE,
    ACTION_PROFILE,
    COUNTER,
    METER,
    REGISTER,
    LEARNING_LIST
  };

  class ForceArith {
    friend class P4Objects;

   public:
    void add_field(const std::string &header_name,
                   const std::string &field_name) {
      fields.insert(std::make_pair(header_name, field_name));
    }

    void add_header(const std::string &header_name) {
      headers.insert(header_name);
    }

   private:
    std::set<header_field_pair> fields;
    std::set<std::string> headers;
  };

 public:
  // NOLINTNEXTLINE(runtime/references)
  explicit P4Objects(std::ostream &outstream, bool verbose_output = false);
  P4Objects();

  int init_objects(std::istream *is,
                   LookupStructureFactory *lookup_factory,
                   device_id_t device_id = 0, cxt_id_t cxt_id = 0,
                   std::shared_ptr<TransportIface> transport = nullptr,
                   const std::set<header_field_pair> &required_fields =
                     std::set<header_field_pair>(),
                   const ForceArith &arith_objects = ForceArith());

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;

 public:
  PHVFactory &get_phv_factory() { return phv_factory; }

  LearnEngineIface *get_learn_engine() { return learn_engine.get(); }

  std::shared_ptr<TransportIface> get_notifications_transport() {
    return notifications_transport;
  }

  AgeingMonitorIface *get_ageing_monitor() { return ageing_monitor.get(); }

  void reset_state();

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in);

  enum class IdLookupErrorCode {
    SUCCESS,
    INVALID_RESOURCE_TYPE,
    INVALID_RESOURCE_NAME
  };

  IdLookupErrorCode id_from_name(ResourceType type, const std::string &name,
                                 p4object_id_t *id);

  ActionFn *get_action_by_id(p4object_id_t id) const {
    return actions_map.at(id).get();
  }

  // TODO(antonin): temporary function to ensure backwards compat of JSON
  ActionFn *get_one_action_with_name(const std::string &name) const {
    for (auto it = actions_map.begin(); it != actions_map.end(); it++) {
      if (it->second->get_name() == name) return it->second.get();
    }
    return nullptr;
  }

  ActionFn *get_action(const std::string &table_name,
                       const std::string &action_name) const {
    return t_actions_map.at(std::make_pair(table_name, action_name));
  }

  ActionFn *get_action_for_action_profile(
      const std::string &act_prof_name, const std::string &action_name) const;

  ActionFn *get_action_rt(const std::string &table_name,
                          const std::string &action_name) const;

  ActionFn *get_action_for_action_profile_rt(
      const std::string &act_prof_name, const std::string &action_name) const;

  // For most functions I have a get_* version that will throw an exception if
  // an element does not exist (exception not caught) and a get_*_rt version
  // that returns a nullptr if it does not exist. I should probably get rid of
  // the first version...
  Parser *get_parser(const std::string &name) const {
    return parsers.at(name).get();
  }

  Parser *get_parser_rt(const std::string &name) const;

  ParseVSet *get_parse_vset(const std::string &name) const {
    return parse_vsets.at(name).get();
  }

  ParseVSet *get_parse_vset_rt(const std::string &name) const;

  Deparser *get_deparser(const std::string &name) const {
    return deparsers.at(name).get();
  }

  Deparser *get_deparser_rt(const std::string &name) const;

  MatchTableAbstract *get_abstract_match_table(const std::string &name) const {
    return match_action_tables_map.at(name)->get_match_table();
  }

  MatchTableAbstract *get_abstract_match_table_rt(
      const std::string &name) const;

  MatchActionTable *get_match_action_table(const std::string &name) const {
    return match_action_tables_map.at(name).get();
  }

  Conditional *get_conditional(const std::string &name) const {
    return conditionals_map.at(name).get();
  }

  ControlFlowNode *get_control_node(const std::string &name) const {
    return control_nodes_map.at(name);
  }

  Pipeline *get_pipeline(const std::string &name) const {
    return pipelines_map.at(name).get();
  }

  Pipeline *get_pipeline_rt(const std::string &name) const;

  MeterArray *get_meter_array(const std::string &name) const {
    return meter_arrays.at(name).get();
  }

  MeterArray *get_meter_array_rt(const std::string &name) const;

  CounterArray *get_counter_array(const std::string &name) const {
    return counter_arrays.at(name).get();
  }

  CounterArray *get_counter_array_rt(const std::string &name) const;

  RegisterArray *get_register_array(const std::string &name) const {
    return register_arrays.at(name).get();
  }

  RegisterArray *get_register_array_rt(const std::string &name) const;

  NamedCalculation *get_named_calculation(const std::string &name) const {
    return calculations.at(name).get();
  }

  FieldList *get_field_list(const p4object_id_t field_list_id) const {
    return field_lists.at(field_list_id).get();
  }

  ExternType *get_extern_instance(const std::string &name) const {
    return extern_instances.at(name).get();
  }

  ExternType *get_extern_instance_rt(const std::string &name) const;

  ActionProfile *get_action_profile(const std::string &name) const {
    return action_profiles_map.at(name).get();
  }

  ActionProfile *get_action_profile_rt(const std::string &name) const;

  bool field_exists(const std::string &header_name,
                    const std::string &field_name) const;

  bool header_exists(const std::string &header_name) const;

  // public to be accessed by test class
  ActionPrimitive_ *get_primitive(const std::string &name);

  ConfigOptionMap get_config_options() const;

  ErrorCodeMap get_error_codes() const;

  EnumMap::type_t get_enum_value(const std::string &name) const;
  const std::string &get_enum_name(const std::string &enum_name,
                                   EnumMap::type_t entry_value) const;

  // returns maximum bmv2 json version supported by this parser as
  // "<major>.<minor>"
  static std::string get_json_version_string();

 private:
  // The get_*_cfg are used during json parsing: they will throw a
  // json_exception if the name cannot be resolved, which will lead to an error
  // message being displayed to the user.
  void add_header_type(const std::string &name,
                       std::unique_ptr<HeaderType> header_type);

  HeaderType *get_header_type_cfg(const std::string &name);

  void add_header_id(const std::string &name, header_id_t header_id);

  void add_header_stack_id(const std::string &name,
                           header_stack_id_t header_stack_id);

  void add_header_union_id(const std::string &name,
                           header_union_id_t header_union_id);

  void add_header_union_stack_id(const std::string &name,
                                 header_union_stack_id_t header_union_stack_id);

  header_id_t get_header_id_cfg(const std::string &name) const;

  header_stack_id_t get_header_stack_id_cfg(const std::string &name) const;

  header_union_id_t get_header_union_id_cfg(const std::string &name) const;

  header_union_stack_id_t get_header_union_stack_id_cfg(
      const std::string &name) const;

  void add_action(p4object_id_t id, std::unique_ptr<ActionFn> action);

  void add_action_to_table(const std::string &table_name,
                           const std::string &action_name, ActionFn *action);

  void add_action_to_act_prof(const std::string &act_prof_name,
                              const std::string &action_name,
                              ActionFn *action);

  void add_parser(const std::string &name, std::unique_ptr<Parser> parser);

  void add_parse_vset(const std::string &name,
                      std::unique_ptr<ParseVSet> parse_vset);

  ParseVSet *get_parse_vset_cfg(const std::string &name) const;

  void add_deparser(const std::string &name,
                    std::unique_ptr<Deparser> deparser);

  void add_match_action_table(const std::string &name,
                              std::unique_ptr<MatchActionTable> table);

  void add_action_profile(const std::string &name,
                          std::unique_ptr<ActionProfile> action_profile);

  ActionProfile *get_action_profile_cfg(const std::string &name) const;

  void add_conditional(const std::string &name,
                       std::unique_ptr<Conditional> conditional);

  void add_control_action(const std::string &name,
                          std::unique_ptr<ControlAction> control_action);

  void add_control_node(const std::string &name, ControlFlowNode *node);

  ControlFlowNode *get_control_node_cfg(const std::string &name) const;

  void add_pipeline(const std::string &name,
                    std::unique_ptr<Pipeline> pipeline);

  void add_meter_array(const std::string &name,
                       std::unique_ptr<MeterArray> meter_array);

  MeterArray *get_meter_array_cfg(const std::string &name) const;

  void add_counter_array(const std::string &name,
                         std::unique_ptr<CounterArray> counter_array);

  CounterArray *get_counter_array_cfg(const std::string &name) const;

  void add_register_array(const std::string &name,
                          std::unique_ptr<RegisterArray> register_array);

  RegisterArray *get_register_array_cfg(const std::string &name) const;

  void add_named_calculation(const std::string &name,
                             std::unique_ptr<NamedCalculation> calculation);

  NamedCalculation *get_named_calculation_cfg(const std::string &name) const;

  void add_field_list(const p4object_id_t field_list_id,
                      std::unique_ptr<FieldList> field_list);

  void add_extern_instance(const std::string &name,
                           std::unique_ptr<ExternType> extern_instance);

  ExternType *get_extern_instance_cfg(const std::string &name) const;

  struct InitState;
  void init_enums(const Json::Value &root);
  void init_header_types(const Json::Value &root);
  void init_headers(const Json::Value &root);
  void init_header_stacks(const Json::Value &root);
  void init_header_unions(const Json::Value &root, InitState *);
  void init_header_union_stacks(const Json::Value &root, InitState *);
  void init_extern_instances(const Json::Value &root);
  void init_parse_vsets(const Json::Value &root);
  void init_errors(const Json::Value &root);
  void init_parsers(const Json::Value &root, InitState *);
  void init_deparsers(const Json::Value &root);
  void init_calculations(const Json::Value &root);
  void init_counter_arrays(const Json::Value &root);
  void init_meter_arrays(const Json::Value &root, InitState *);
  void init_register_arrays(const Json::Value &root);
  void init_actions(const Json::Value &root);
  void init_pipelines(const Json::Value &root, LookupStructureFactory *,
                      InitState *);
  void init_checksums(const Json::Value &root);
  void init_learn_lists(const Json::Value &root);
  void init_field_lists(const Json::Value &root);

  void build_expression(const Json::Value &json_expression, Expression *expr);
  void build_expression(const Json::Value &json_expression, Expression *expr,
                        ExprType *expr_type);

  void add_primitive_to_action(const Json::Value &primitive,
                               ActionFn *action_fn);
  void process_single_param(ActionFn* action_fn,
                            const Json::Value &cfg_parameter,
                            const std::string &primitive_name);

  void parse_config_options(const Json::Value &root);

 private:
  PHVFactory phv_factory{};  // this is probably temporary

  std::unordered_map<std::string, header_id_t> header_ids_map{};
  std::unordered_map<std::string, header_stack_id_t> header_stack_ids_map{};
  std::unordered_map<std::string, header_union_id_t> header_union_ids_map{};
  std::unordered_map<std::string, header_union_stack_id_t>
  header_union_stack_ids_map{};
  std::unordered_map<std::string, HeaderType *> header_to_type_map{};
  std::unordered_map<std::string, HeaderType *> header_stack_to_type_map{};

  std::unordered_map<std::string, std::unique_ptr<HeaderType> >
  header_types_map{};

  // tables
  std::unordered_map<std::string, std::unique_ptr<MatchActionTable> >
  match_action_tables_map{};

  std::unordered_map<std::string, std::unique_ptr<ActionProfile> >
  action_profiles_map{};

  std::unordered_map<std::string, std::unique_ptr<Conditional> >
  conditionals_map{};

  std::unordered_map<std::string, std::unique_ptr<ControlAction> >
  control_actions_map{};

  std::unordered_map<std::string, ControlFlowNode *> control_nodes_map{};

  // pipelines
  std::unordered_map<std::string, std::unique_ptr<Pipeline> > pipelines_map{};

  // actions
  // TODO(antonin): make this a vector?
  std::unordered_map<p4object_id_t, std::unique_ptr<ActionFn> > actions_map{};
  using table_action_pair = std::pair<std::string, std::string>;
  struct TableActionPairKeyHash {
    std::size_t operator()(const table_action_pair& p) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, p.first);
      boost::hash_combine(seed, p.second);

      return seed;
    }
  };
  std::unordered_map<table_action_pair, ActionFn *, TableActionPairKeyHash>
  t_actions_map{};
  using aprof_action_pair = table_action_pair;
  using AprofActionPairKeyHash = TableActionPairKeyHash;
  std::unordered_map<aprof_action_pair, ActionFn *, AprofActionPairKeyHash>
  aprof_actions_map{};

  // parsers
  std::unordered_map<std::string, std::unique_ptr<Parser> > parsers{};
  // this is to give the objects a place where to live
  std::vector<std::unique_ptr<ParseState> > parse_states{};
  // this is to give ActionFn objects a place to live
  std::vector<std::unique_ptr<ActionFn> > parse_methods{};
  std::vector<std::unique_ptr<ActionFn> > deparse_methods{};

  // parse vsets
  std::unordered_map<std::string, std::unique_ptr<ParseVSet> > parse_vsets{};

  ErrorCodeMap error_codes;

  EnumMap enums{};

  // checksums
  std::vector<std::unique_ptr<Checksum> > checksums{};

  std::unordered_map<std::string, std::unique_ptr<Deparser> > deparsers{};

  std::unique_ptr<LearnEngineIface> learn_engine{};
  // for now, used exclusively as a way to map list names to ids
  class LearnList : public NamedP4Object {
   public:
    LearnList(const std::string &name, p4object_id_t id)
        : NamedP4Object(name, id) { }
  };
  std::unordered_map<std::string, std::unique_ptr<LearnList> > learn_lists{};

  std::shared_ptr<TransportIface> notifications_transport{};

  std::unique_ptr<AgeingMonitorIface> ageing_monitor{};

  // meter arrays
  std::unordered_map<std::string, std::unique_ptr<MeterArray> > meter_arrays{};

  // counter arrays
  std::unordered_map<std::string, std::unique_ptr<CounterArray> >
    counter_arrays{};

  // register arrays
  std::unordered_map<std::string, std::unique_ptr<RegisterArray> >
    register_arrays{};

  // calculations
  std::unordered_map<std::string, std::unique_ptr<NamedCalculation> >
    calculations{};

  // field lists
  std::unordered_map<p4object_id_t, std::unique_ptr<FieldList> > field_lists{};

  // extern instances
  std::unordered_map<std::string, std::unique_ptr<ExternType> >
    extern_instances{};

  std::unordered_map<std::string, header_field_pair> field_aliases{};

  // used for initialization only
  std::unordered_map<p4object_id_t, p4object_id_t> header_id_to_stack_id{};
  struct HeaderUnionPos {
    header_union_id_t union_id;
    size_t offset;  // the offset of the header in the union
  };
  std::unordered_map<p4object_id_t, HeaderUnionPos> header_id_to_union_pos{};
  std::unordered_map<p4object_id_t, header_union_stack_id_t>
  union_id_to_union_stack_id{};

  ConfigOptionMap config_options{};

  // maps primitive names to primitive instances
  std::unordered_map<std::string, std::unique_ptr<ActionPrimitive_>>
      primitives{};

  std::ostream &outstream;
  bool verbose_output;

 private:
  int get_field_offset(header_id_t header_id,
                       const std::string &field_name) const;
  size_t get_field_bytes(header_id_t header_id, int field_offset) const;
  size_t get_field_bits(header_id_t header_id, int field_offset) const;
  size_t get_header_bits(header_id_t header_id) const;
  std::tuple<header_id_t, int> field_info(const std::string &header_name,
                                          const std::string &field_name) const;
  bool check_required_fields(
      const std::set<header_field_pair> &required_fields);

  std::unique_ptr<CalculationsMap::MyC> check_hash(
      const std::string &name) const;

  void enable_arith(header_id_t header_id, int field_offset);
  void enable_arith(header_id_t header_id);

  std::unique_ptr<Calculation> process_cfg_selector(
      const Json::Value &cfg_selector) const;
};

}  // namespace bm

#endif  // BM_BM_SIM_P4OBJECTS_H_
