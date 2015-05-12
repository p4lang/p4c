#ifndef _BM_P4OBJECTS_H_
#define _BM_P4OBJECTS_H_

#include <istream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <set>

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

class P4Objects {
public:
  void init_objects(std::istream &is);
  void destroy_objects();

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;
  P4Objects() = default;

public:
  PHVFactory &get_phv_factory() { return phv_factory; }

  LearnEngine *get_learn_engine() { return learn_engine.get(); }

  ActionFn *get_action(const std::string &name) {
    return actions_map[name].get();
  }

  Parser *get_parser(const std::string &name) {
    return parsers[name].get();
  }

  Deparser *get_deparser(const std::string &name) {
    return deparsers[name].get();
  }

  ExactMatchTable *get_exact_match_table(const std::string &name) {
    return exact_tables_map[name].get();
  }

  LongestPrefixMatchTable *get_lpm_table(const std::string &name) {
    return lpm_tables_map[name].get();
  }

  TernaryMatchTable *get_ternary_match_table(const std::string &name) {
    return ternary_tables_map[name].get();
  }

  MatchTable *get_match_table(const std::string &name) {
    return tables_map[name];
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

  header_id_t get_header_id(const std::string &name) {
    return header_ids_map[name];
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

  void add_exact_match_table(const std::string &name,
			     std::unique_ptr<ExactMatchTable> table) {
    add_match_table(name, table.get());
    exact_tables_map[name] = std::move(table);
  }

  void add_lpm_table(const std::string &name,
		     std::unique_ptr<LongestPrefixMatchTable> table) {
    add_match_table(name, table.get());
    lpm_tables_map[name] = std::move(table);
  }

  void add_ternary_match_table(const std::string &name,
			       std::unique_ptr<TernaryMatchTable> table) {
    add_match_table(name, table.get());
    ternary_tables_map[name] = std::move(table);
  }

  void add_match_table(const std::string &name, MatchTable *table) {
    tables_map[name] = table;
    add_control_node(name, table);
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

  void build_conditional(const Json::Value &json_expression,
			 Conditional *conditional);

  std::set<int> build_arith_offsets(const Json::Value &json_actions,
				    const std::string &header_name);

private:
  PHVFactory phv_factory{}; /* this is probably temporary */

  std::unordered_map<std::string, header_id_t> header_ids_map{};
  std::unordered_map<std::string, HeaderType *> header_to_type_map{};

  std::unordered_map<std::string, std::unique_ptr<HeaderType> > header_types_map{};

  // tables
  std::unordered_map<std::string, std::unique_ptr<ExactMatchTable> > exact_tables_map{};
  std::unordered_map<std::string, std::unique_ptr<LongestPrefixMatchTable> > lpm_tables_map{};
  std::unordered_map<std::string, std::unique_ptr<TernaryMatchTable> > ternary_tables_map{};
  std::unordered_map<std::string, MatchTable *> tables_map{};

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

private:
  int get_field_offset(header_id_t header_id, const std::string &field_name);
  size_t get_field_bytes(header_id_t header_id, int field_offset);
};


#endif
