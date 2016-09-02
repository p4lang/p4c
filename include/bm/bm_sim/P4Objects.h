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

#include <istream>
#include <ostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <set>
#include <tuple>
#include <utility>  // for pair<>

#include "tables.h"
#include "headers.h"
#include "phv.h"
#include "parser.h"
#include "deparser.h"
#include "pipeline.h"
#include "conditionals.h"
#include "control_flow.h"
#include "learning.h"
#include "meters.h"
#include "counters.h"
#include "stateful.h"
#include "ageing.h"
#include "field_lists.h"
#include "extern.h"

// forward declaration of Json::Value
namespace Json {

class Value;

}  // namespace Json

namespace bm {

class P4Objects {
 public:
  typedef std::pair<std::string, std::string> header_field_pair;

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
  // A reference works great here, but should I switch to a pointer?
  // NOLINTNEXTLINE(runtime/references)
  explicit P4Objects(std::ostream &outstream = std::cout)
      : outstream(outstream) { }

  int init_objects(std::istream *is,
                   LookupStructureFactory * lookup_factory,
                   int device_id = 0, size_t cxt_id = 0,
                   std::shared_ptr<TransportIface> transport = nullptr,
                   const std::set<header_field_pair> &required_fields =
                     std::set<header_field_pair>(),
                   const ForceArith &arith_objects = ForceArith());

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;

 public:
  PHVFactory &get_phv_factory() { return phv_factory; }

  LearnEngine *get_learn_engine() { return learn_engine.get(); }

  AgeingMonitor *get_ageing_monitor() { return ageing_monitor.get(); }

  void reset_state();

  void serialize(std::ostream *out) const;
  void deserialize(std::istream *in);

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

  Parser *get_parser(const std::string &name) const {
    return parsers.at(name).get();
  }

  ParseVSet *get_parse_vset(const std::string &name) const {
    return parse_vsets.at(name).get();
  }

  ParseVSet *get_parse_vset_rt(const std::string &name) const;

  Deparser *get_deparser(const std::string &name) const {
    return deparsers.at(name).get();
  }

  MatchTableAbstract *get_abstract_match_table(const std::string &name) const {
    return match_action_tables_map.at(name)->get_match_table();
  }

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

  bool field_exists(const std::string &header_name,
                    const std::string &field_name) const;

  bool header_exists(const std::string &header_name) const;

 private:
  void add_header_type(const std::string &name,
                         std::unique_ptr<HeaderType> header_type) {
    header_types_map[name] = std::move(header_type);
  }

  HeaderType *get_header_type(const std::string &name) {
    return header_types_map.at(name).get();
  }

  void add_header_id(const std::string &name, header_id_t header_id) {
    header_ids_map[name] = header_id;
  }

  void add_header_stack_id(const std::string &name,
                           header_stack_id_t header_stack_id) {
    header_stack_ids_map[name] = header_stack_id;
  }

  header_id_t get_header_id(const std::string &name) {
    return header_ids_map.at(name);
  }

  header_stack_id_t get_header_stack_id(const std::string &name) {
    return header_stack_ids_map.at(name);
  }

  void add_action(p4object_id_t id, std::unique_ptr<ActionFn> action) {
    actions_map[id] = std::move(action);
  }

  void add_action_to_table(const std::string &table_name,
                           const std::string &action_name, ActionFn *action) {
    t_actions_map[std::make_pair(table_name, action_name)] = action;
  }

  void add_parser(const std::string &name, std::unique_ptr<Parser> parser) {
    parsers[name] = std::move(parser);
  }

  void add_parse_vset(const std::string &name,
                      std::unique_ptr<ParseVSet> parse_vset) {
    parse_vsets[name] = std::move(parse_vset);
  }

  void add_deparser(const std::string &name,
                    std::unique_ptr<Deparser> deparser) {
    deparsers[name] = std::move(deparser);
  }

  void add_match_action_table(const std::string &name,
                              std::unique_ptr<MatchActionTable> table) {
    add_control_node(name, table.get());
    match_action_tables_map[name] = std::move(table);
  }

  void add_conditional(const std::string &name,
                       std::unique_ptr<Conditional> conditional) {
    add_control_node(name, conditional.get());
    conditionals_map[name] = std::move(conditional);
  }

  void add_control_node(const std::string &name, ControlFlowNode *node) {
    control_nodes_map[name] = node;
  }

  void add_pipeline(const std::string &name,
                    std::unique_ptr<Pipeline> pipeline) {
    pipelines_map[name] = std::move(pipeline);
  }

  void add_meter_array(const std::string &name,
                       std::unique_ptr<MeterArray> meter_array) {
    meter_arrays[name] = std::move(meter_array);
  }

  void add_counter_array(const std::string &name,
                         std::unique_ptr<CounterArray> counter_array) {
    counter_arrays[name] = std::move(counter_array);
  }

  void add_register_array(const std::string &name,
                          std::unique_ptr<RegisterArray> register_array) {
    register_arrays[name] = std::move(register_array);
  }

  void add_named_calculation(const std::string &name,
                             std::unique_ptr<NamedCalculation> calculation) {
    calculations[name] = std::move(calculation);
  }

  void add_field_list(const p4object_id_t field_list_id,
                      std::unique_ptr<FieldList> field_list) {
    field_lists[field_list_id] = std::move(field_list);
  }

  void add_extern_instance(const std::string &name,
                           std::unique_ptr<ExternType> extern_instance) {
    extern_instances[name] = std::move(extern_instance);
  }

  void build_expression(const Json::Value &json_expression, Expression *expr);

  std::set<int> build_arith_offsets(const Json::Value &json_actions,
                                    const std::string &header_name);

 private:
  PHVFactory phv_factory{};  // this is probably temporary

  std::unordered_map<std::string, header_id_t> header_ids_map{};
  std::unordered_map<std::string, header_stack_id_t> header_stack_ids_map{};
  std::unordered_map<std::string, HeaderType *> header_to_type_map{};
  std::unordered_map<std::string, HeaderType *> header_stack_to_type_map{};

  std::unordered_map<std::string, std::unique_ptr<HeaderType> >
  header_types_map{};

  // tables
  std::unordered_map<std::string, std::unique_ptr<MatchActionTable> >
    match_action_tables_map{};

  std::unordered_map<std::string, std::unique_ptr<Conditional> >
    conditionals_map{};

  std::unordered_map<std::string, ControlFlowNode *> control_nodes_map{};

  // pipelines
  std::unordered_map<std::string, std::unique_ptr<Pipeline> > pipelines_map{};

  // actions
  // TODO(antonin): make this a vector?
  std::unordered_map<p4object_id_t, std::unique_ptr<ActionFn> > actions_map{};
  typedef std::pair<std::string, std::string> table_action_pair;
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
  // std::unordered_map<std::string, std::unique_ptr<ActionFn> > actions_map{};

  // parsers
  std::unordered_map<std::string, std::unique_ptr<Parser> > parsers{};
  // this is to give the objects a place where to live
  std::vector<std::unique_ptr<ParseState> > parse_states{};

  // parse vsets
  std::unordered_map<std::string, std::unique_ptr<ParseVSet> > parse_vsets{};

  // checksums
  std::vector<std::unique_ptr<Checksum> > checksums{};

  std::unordered_map<std::string, std::unique_ptr<Deparser> > deparsers{};

  std::unique_ptr<LearnEngine> learn_engine{};

  std::unique_ptr<AgeingMonitor> ageing_monitor{};

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

 public:
  // public to be accessed by test class
  std::ostream &outstream;

 private:
  int get_field_offset(header_id_t header_id, const std::string &field_name);
  size_t get_field_bytes(header_id_t header_id, int field_offset);
  size_t get_field_bits(header_id_t header_id, int field_offset);
  size_t get_header_bits(header_id_t header_id);
  std::tuple<header_id_t, int> field_info(const std::string &header_name,
                                          const std::string &field_name);
  bool check_required_fields(
      const std::set<header_field_pair> &required_fields);

  std::unique_ptr<CalculationsMap::MyC> check_hash(
      const std::string &name) const;
};

}  // namespace bm

#endif  // BM_BM_SIM_P4OBJECTS_H_
