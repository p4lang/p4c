#ifndef _BM_PARSER_H_
#define _BM_PARSER_H_

#include <utility>
#include <iostream>
#include <string>

#include <cassert>

#include "packet.h"
#include "phv.h"
#include "named_p4object.h"
#include "event_logger.h"

using std::pair;
using std::vector;
using std::string;

struct ParserOp {
  virtual ~ParserOp() {};
  virtual void operator()(Packet *pkt, const char *data,
			  size_t *bytes_parsed) const = 0;
};

struct ParserOpExtract : ParserOp {
  header_id_t header;

  ParserOpExtract(header_id_t header)
    : header(header) {}

  ~ParserOpExtract() {}

  void operator()(Packet *pkt, const char *data,
		  size_t *bytes_parsed) const
  {
    PHV *phv = pkt->get_phv();
    ELOGGER->parser_extract(*pkt, header);
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

  void push_back_field(header_id_t header, int field_offset) {
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
  ByteContainer key;
  ByteContainer mask;
  bool with_mask;
  const ParseState *next_state; /* NULL if end */

public:
  ParseSwitchCase(const ByteContainer &key, const ParseState *next_state)
    : key(key), with_mask(false), next_state(next_state) {
  }

  ParseSwitchCase(int nbytes_key, const char *key, const ParseState *next_state)
    : key(ByteContainer(key, nbytes_key)),
      with_mask(false), next_state(next_state) {
  }
  
  ParseSwitchCase(const ByteContainer &key,
		  const ByteContainer &mask,
		  const ParseState *next_state)
    : key(key), mask(mask), with_mask(true), next_state(next_state) {
	assert(key.size() == mask.size());
  }

  bool match(const ByteContainer &input, const ParseState **state) const;
};

class ParseState {
private:
  string name;
  vector<ParserOp *> parser_ops;
  bool has_switch;
  ParseSwitchKeyBuilder key_builder;
  vector<ParseSwitchCase> parser_switch;
  const ParseState *default_next_state{nullptr};

public:
  ParseState(string name)
    : name(name), has_switch(false) {}

  void add_extract(header_id_t header) {
    ParserOp *parser_op = new ParserOpExtract(header);
    parser_ops.push_back(parser_op);
  }

  void set_key_builder(const ParseSwitchKeyBuilder &builder) {
    has_switch = true;
    key_builder = builder;
  }

  void add_switch_case(const ByteContainer &key, const ParseState *next_state) {
    parser_switch.push_back( ParseSwitchCase(key, next_state) );
  }

  void add_switch_case(int nbytes_key, const char *key,
		       const ParseState *next_state) {
    parser_switch.push_back( ParseSwitchCase(nbytes_key, key, next_state) );
  }

  void set_default_switch_case(const ParseState *default_next) {
    default_next_state = default_next;
  }

  const string &get_name() const { return name; }

  ~ParseState() {
    for (vector<ParserOp *>::iterator it = parser_ops.begin();
	 it != parser_ops.end();
	 ++it) {
      delete *it;
    }
  }

  // Copy constructor
  ParseState (const ParseState& other) = delete;

  // Copy assignment operator
  ParseState &operator =(const ParseState& other) = delete;
 
  // Move constructor
  ParseState(ParseState&& other)= default;
 
  // Move assignment operator
  ParseState &operator =(ParseState &&other) = default;

  const ParseState *operator()(Packet *pkt, const char *data,
			       size_t *bytes_parsed) const;
};

class Parser : public NamedP4Object {
public:
 Parser(const std::string &name, p4object_id_t id)
   : NamedP4Object(name, id), init_state(NULL) {}

  void set_init_state(const ParseState *state) {
    init_state = state;
  }

  void parse(Packet *pkt) const;

private:
  const ParseState *init_state;
};

#endif
