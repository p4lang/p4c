#include <istream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "tables.h"
#include "headers.h"
#include "phv.h"
#include "parser.h"
#include "deparser.h"

using std::vector;
using std::unordered_map;
using std::string;
using std::unique_ptr;

class P4Objects {
public:
  void init_objects(std::istream &is);
  void destroy_objects();

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;
  P4Objects() = default;

private:
  void add_header_type(const string &name, unique_ptr<HeaderType> header_type) {
    header_types_map[name] = std::move(header_type);
  }

  HeaderType *get_header_type(const string &name) {
    return header_types_map[name].get();
  }

  void add_header_id(const string &name, header_id_t header_id) {
    header_ids_map[name] = header_id;
  }

  header_id_t get_header_id(const string &name) {
    return header_ids_map[name];
  }

  void add_action(const string &name, unique_ptr<ActionFn> action) {
    actions_map[name] = std::move(action);
  }

  ActionFn *get_action(const string &name) {
    return actions_map[name].get();
  }

  void add_parser(const string &name, unique_ptr<Parser> parser) {
    parsers[name] = std::move(parser);
  }

  Parser *get_parser(const string &name) {
    return parsers[name].get();
  }

  void add_deparser(const string &name, unique_ptr<Deparser> deparser) {
    deparsers[name] = std::move(deparser);
  }

  Deparser *get_deparser(const string &name) {
    return deparsers[name].get();
  }

private:
  PHV phv;

  unordered_map<string, header_id_t> header_ids_map;

  unordered_map<string, unique_ptr<HeaderType> > header_types_map;

  // tables
  unordered_map<string, unique_ptr<ExactMatchTable> > exact_tables_map;
  unordered_map<string, unique_ptr<LongestPrefixMatchTable> > lpm_tables_map;
  unordered_map<string, unique_ptr<TernaryMatchTable> > ternary_tables_map;

  // actions
  unordered_map<string, unique_ptr<ActionFn> > actions_map;

  // parsers
  unordered_map<string, unique_ptr<Parser> > parsers;
  // this is to give the objects a place where to live
  vector<unique_ptr<ParseState> > parse_states;

  unordered_map<string, unique_ptr<Deparser> > deparsers;

private:
  int get_field_offset(header_id_t header_id, const string &field_name);
};
