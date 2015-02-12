#ifndef _BM_PARSER_H_
#define _BM_PARSER_H_

#include <utility>
#include <iostream>

#include "packet.h"
#include "phv.h"

using std::pair;
using std::vector;

struct ParserOp {
  virtual ~ParserOp() {};
  virtual void operator()(const char *data,
			  PHV *phv, size_t *bytes_parsed) const = 0;
};

struct ParserOpExtract : ParserOp {
  header_id_t header;

  ParserOpExtract(header_id_t header)
    : header(header) {}

  ~ParserOpExtract() {}

  void operator()(const char *data,
		  PHV *phv, size_t *bytes_parsed) const
  {
    Header &hdr = phv->get_header(header);
    hdr.extract(data);
    *bytes_parsed += hdr.get_nbytes_packet();
  }
};

struct ParserOpSet : ParserOp {
  // TODO

  ~ParserOpSet() {}
};

struct ParseSwitchKeyBuilder
{
  vector< pair<header_id_t, int> > fields;

  void add_field(header_id_t header, int field_offset) {
    fields.push_back( pair<header_id_t, int>(header, field_offset) );
  }
  
  // data not used for now
  void operator()(const PHV &phv, const char *data, ByteContainer &key) const
  {
    for(vector< pair<header_id_t, int> >::const_iterator it = fields.begin();
	it != fields.end();
	it++) {
      const Field &field = phv.get_field((*it).first, (*it).second);
      key.append(field.get_bytes());
    }
  }

  ParseSwitchKeyBuilder& operator=(const ParseSwitchKeyBuilder &other) {
    if(&other == this)
      return *this;
    fields = other.fields;
    return *this;
  }
};

class ParseState;

class ParseSwitchCase {
private:
  int nbytes_key;
  ByteContainer key;
  ByteContainer mask;
  bool with_mask;
  const ParseState *next_state; /* NULL if end */

public:
  ParseSwitchCase(int nbytes_key, const ByteContainer &key,
		  const ParseState *next_state)
    : nbytes_key(nbytes_key), key(key), with_mask(false),
      next_state(next_state) {
  }

  ParseSwitchCase(int nbytes_key, const char *key,
		  const ParseState *next_state)
    : nbytes_key(nbytes_key), key(ByteContainer(key, nbytes_key)),
      with_mask(false), next_state(next_state) {
  }

  ParseSwitchCase(int nbytes_key, const ByteContainer &key,
		  const ByteContainer &mask,
		  const ParseState *next_state)
    : nbytes_key(nbytes_key), key(key), mask(mask), with_mask(true),
      next_state(next_state) {
  }

  bool match(const ByteContainer &input, const ParseState **state) const;
};

class ParseState {
private:
  vector<ParserOp *> parser_ops;
  int nbytes_key;
  bool has_switch;
  ParseSwitchKeyBuilder key_builder;
  vector<ParseSwitchCase> parser_switch;

public:
  ParseState()
    : has_switch(false) {}

  void add_extract(header_id_t header) {
    ParserOp *parser_op = new ParserOpExtract(header);
    // std::cout << parser_op << std::endl;
    parser_ops.push_back(parser_op);
    // std::cout << parser_op << std::endl;
  }

  void set_key_builder(int nbytes, const ParseSwitchKeyBuilder &builder) {
    has_switch = true;
    key_builder = builder;
    nbytes_key = nbytes;
  }

  void add_switch_case(const ByteContainer &key, const ParseState *next_state) {
    parser_switch.push_back( ParseSwitchCase(nbytes_key, key, next_state) );
  }

  void add_switch_case(const char *key, const ParseState *next_state) {
    parser_switch.push_back( ParseSwitchCase(nbytes_key, key, next_state) );
  }

  ~ParseState() {
    for (vector<ParserOp *>::iterator it = parser_ops.begin();
	 it != parser_ops.end();
	 ++it) {
      delete *it;
    }
  }

  const ParseState *operator()(const char *data,
			       PHV *phv, size_t *bytes_parsed) const;
};

class Parser {
private:
  const ParseState *init_state;
  
public:
  Parser()
    : init_state(NULL) {}

  Parser(const ParseState *init_state)
    : init_state(init_state) {}

  void set_init_state(const ParseState *state) {
    init_state = state;
  }

  void parse(Packet *pkt, PHV *phv) const;
};

#endif
