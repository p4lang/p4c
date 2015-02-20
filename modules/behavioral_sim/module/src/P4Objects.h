#include <istream>
#include <vector>
#include <unordered_map>
#include <string>

#include "tables.h"
#include "headers.h"
#include "phv.h"
#include "parser.h"
#include "deparser.h"

using std::vector;
using std::unordered_map;
using std::string;

class P4Objects {
public:
  void init_objects(std::istream &is);
  void destroy_objects();

  P4Objects(const P4Objects &other) = delete;
  P4Objects &operator=(const P4Objects &) = delete;
  P4Objects() = default;

private:
  void add_header_type(const string &name, HeaderType *header_type) {
    header_types_map[name] = header_type;
  }

  HeaderType *get_header_type(const string &name) {
    return header_types_map[name];
  }

  void add_header_id(const string &name, header_id_t header_id) {
    header_ids_map[name] = header_id;
  }

  header_id_t get_header_id(const string &name) {
    return header_ids_map[name];
  }

  void add_parser(const string &name, Parser *parser) {
    parsers[name] = parser;
  }

  Parser *get_parser(const string &name) {
    return parsers[name];
  }

  void add_deparser(const string &name, Deparser *deparser) {
    deparsers[name] = deparser;
  }

  Deparser *get_deparser(const string &name) {
    return deparsers[name];
  }

private:
  PHV phv;

  unordered_map<string, header_id_t> header_ids_map;

  unordered_map<string, HeaderType *> header_types_map;

  // tables
  unordered_map<string, ExactMatchTable *> exact_tables_map;
  unordered_map<string, LongestPrefixMatchTable *> lpm_tables_map;
  unordered_map<string, TernaryMatchTable *> ternary_tables_map;

  // parsers
  unordered_map<string, Parser *> parsers;
  // this is to give the objects a place where to live
  vector<ParseState *> parse_states;

  unordered_map<string, Deparser *> deparsers;
};
