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

#ifndef _BM_P4OBJECTS_H_
#define _BM_P4OBJECTS_H_

#include <istream>
#include <ostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <set>
#include <tuple>

#include <jsoncpp/json.h>

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

class P4Objects {
public:
  typedef std::pair<std::string, std::string> header_field_pair;

public:
  P4Objects(std::ostream &outstream = std::cout)
    : outstream(outstream) { }

  int init_objects(std::istream &is,
		   const std::set<header_field_pair> &required_fields = std::set<header_field_pair>(),
		   const std::set<header_field_pair> &arith_fields = std::set<header_field_pair>());
  void destroy_objects();

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;

public:
  PHVFactory &get_phv_factory() { return phv_factory; }

  LearnEngine *get_learn_engine() { return learn_engine.get(); }

  AgeingMonitor *get_ageing_monitor() { return ageing_monitor.get(); }

  void reset_state();

  ActionFn *get_action(const std::string &name) {
    return actions_map[name].get();
  }

  Parser *get_parser(const std::string &name) {
    return parsers[name].get();
  }

  Deparser *get_deparser(const std::string &name) {
    return deparsers[name].get();
  }

  MatchTableAbstract *get_abstract_match_table(const std::string &name) {
    return match_action_tables_map[name]->get_match_table();
  }

  MatchActionTable *get_match_action_table(const std::string &name) {
    return match_action_tables_map[name].get();
  }

  Conditional *get_conditional(const std::string &name) {
    return conditionals_map[name].get();
  }

  ControlFlowNode *get_control_node(const std::string &name) {
    return control_nodes_map[name];
  }

  Pipeline *get_pipeline(const std::string &name) {
    return pipelines_map[name].get();
  }

  MeterArray *get_meter_array(const std::string &name) {
    return meter_arrays[name].get();
  }

  CounterArray *get_counter_array(const std::string &name) {
    return counter_arrays[name].get();
  }

  RegisterArray *get_register_array(const std::string &name) {
    return register_arrays[name].get();
  }

  NamedCalculation *get_named_calculation(const std::string &name) {
    return calculations[name].get();
  }

  FieldList *get_field_list(const p4object_id_t field_list_id) {
    return field_lists[field_list_id].get();
  }

private:
  void add_header_type(const std::string &name, std::unique_ptr<HeaderType> header_type) {
    header_types_map[name] = std::move(header_type);
  }

  HeaderType *get_header_type(const std::string &name) {
    return header_types_map[name].get();
  }

  void add_header_id(const std::string &name, header_id_t header_id) {
    header_ids_map[name] = header_id;
  }

  void add_header_stack_id(const std::string &name,
			   header_stack_id_t header_stack_id) {
    header_stack_ids_map[name] = header_stack_id;
  }

  header_id_t get_header_id(const std::string &name) {
    return header_ids_map[name];
  }

  header_stack_id_t get_header_stack_id(const std::string &name) {
    return header_stack_ids_map[name];
  }

  void add_action(const std::string &name, std::unique_ptr<ActionFn> action) {
    actions_map[name] = std::move(action);
  }

  void add_parser(const std::string &name, std::unique_ptr<Parser> parser) {
    parsers[name] = std::move(parser);
  }

  void add_deparser(const std::string &name, std::unique_ptr<Deparser> deparser) {
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

  void add_pipeline(const std::string &name, std::unique_ptr<Pipeline> pipeline) {
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

  void build_expression(const Json::Value &json_expression, Expression *expr);

  std::set<int> build_arith_offsets(const Json::Value &json_actions,
				    const std::string &header_name);

private:
  PHVFactory phv_factory{}; /* this is probably temporary */

  std::unordered_map<std::string, header_id_t> header_ids_map{};
  std::unordered_map<std::string, header_stack_id_t> header_stack_ids_map{};
  std::unordered_map<std::string, HeaderType *> header_to_type_map{};
  std::unordered_map<std::string, HeaderType *> header_stack_to_type_map{};

  std::unordered_map<std::string, std::unique_ptr<HeaderType> > header_types_map{};

  // tables
  std::unordered_map<std::string, std::unique_ptr<MatchActionTable> > match_action_tables_map{};

  std::unordered_map<std::string, std::unique_ptr<Conditional> > conditionals_map{};

  std::unordered_map<std::string, ControlFlowNode *> control_nodes_map{};

  // pipelines
  std::unordered_map<std::string, std::unique_ptr<Pipeline> > pipelines_map{};

  // actions
  std::unordered_map<std::string, std::unique_ptr<ActionFn> > actions_map{};

  // parsers
  std::unordered_map<std::string, std::unique_ptr<Parser> > parsers{};
  // this is to give the objects a place where to live
  std::vector<std::unique_ptr<ParseState> > parse_states{};

  // checksums
  std::vector<std::unique_ptr<Checksum> > checksums{};

  std::unordered_map<std::string, std::unique_ptr<Deparser> > deparsers{};

  std::unique_ptr<LearnEngine> learn_engine{};

  std::unique_ptr<AgeingMonitor> ageing_monitor{};

  // meter arrays
  std::unordered_map<std::string, std::unique_ptr<MeterArray> > meter_arrays{};

  // counter arrays
  std::unordered_map<std::string, std::unique_ptr<CounterArray> > counter_arrays{};

  // register arrays
  std::unordered_map<std::string, std::unique_ptr<RegisterArray> > register_arrays{};

  // calculations
  std::unordered_map<std::string, std::unique_ptr<NamedCalculation> > calculations{};

  // field lists
  std::unordered_map<p4object_id_t, std::unique_ptr<FieldList> > field_lists;

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
  bool field_exists(const std::string &header_name,
		    const std::string &field_name);
  bool check_required_fields(const std::set<header_field_pair> &required_fields);

  std::unique_ptr<CalculationsMap::MyC> check_hash(const std::string &name) const;
};


#endif
