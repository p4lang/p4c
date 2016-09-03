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

//! @file parser.h

#ifndef BM_BM_SIM_PARSER_H_
#define BM_BM_SIM_PARSER_H_

#include <utility>
#include <string>
#include <vector>
#include <mutex>

#include <cassert>

#include "packet.h"
#include "phv.h"
#include "named_p4object.h"
#include "event_logger.h"
#include "logger.h"
#include "expressions.h"
#include "stateful.h"

namespace bm {

struct field_t {
  header_id_t header;
  int offset;

  static field_t make(header_id_t header, int offset) {
    field_t field = {header, offset};
    return field;
  }
};

struct ParserLookAhead {
  int byte_offset;
  int bit_offset;
  int bitwidth;
  size_t nbytes;

  ParserLookAhead(int offset, int bitwidth);

  void peek(const char *data, ByteContainer *res) const;
};

struct ParserOp {
  virtual ~ParserOp() {}
  virtual void operator()(Packet *pkt, const char *data,
                          size_t *bytes_parsed) const = 0;
};

struct ParserOpExtract : ParserOp {
  header_id_t header;

  explicit ParserOpExtract(header_id_t header)
    : header(header) {}

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    PHV *phv = pkt->get_phv();
    Header &hdr = phv->get_header(header);
    BMELOG(parser_extract, *pkt, header);
    BMLOG_DEBUG_PKT(*pkt, "Extracting header '{}'", hdr.get_name());
    hdr.extract(data, *phv);
    *bytes_parsed += hdr.get_nbytes_packet();
  }
};

// push back a header on a tag stack
// TODO(antonin): probably room for improvement here
struct ParserOpExtractStack : ParserOp {
  header_stack_id_t header_stack;

  explicit ParserOpExtractStack(header_stack_id_t header_stack)
    : header_stack(header_stack) {}

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    PHV *phv = pkt->get_phv();
    HeaderStack &stack = phv->get_header_stack(header_stack);
    Header &next_hdr = stack.get_next();  // TODO(antonin): will assert if full
    BMELOG(parser_extract, *pkt, next_hdr.get_id());
    BMLOG_DEBUG_PKT(*pkt, "Extracting to header stack {}, next header is {}",
                    header_stack, next_hdr.get_id());
    next_hdr.extract(data, *phv);
    *bytes_parsed += next_hdr.get_nbytes_packet();
    // should I have a HeaderStack::extract() method instead?
    stack.push_back();
  }
};

template <typename T>
struct ParserOpSet : ParserOp {
  field_t dst;
  T src;

  ParserOpSet(header_id_t header, int offset, const T &src)
    : dst({header, offset}), src(src) {
    // dst = {header, offset};
  }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override;
};

struct ParseSwitchKeyBuilder {
  struct Entry {
    // I could use a union, but I only have 2 choices, and they are both quite
    // small. Plus I make a point of not using unions for non POD data. So I
    // could either change ParserLookAhead to POD data or go through the trouble
    // of implementing a union with setters, a copy assignment operator, a
    // dtor... I'll see if it is worth it later
    // The larger picture: is there really an improvement in performance. versus
    // using derived classes and virtual methods ? I guess there is a small one,
    // not because of the use of the vtable, but because I don't have to store
    // pointers, but I can store the objects in the vector directly
    // After some testing, it appears that using this is at least a couple times
    // faster
    enum {FIELD, LOOKAHEAD, STACK_FIELD} tag{};
    field_t field{0, 0};
    ParserLookAhead lookahead{0, 0};

    static Entry make_field(header_id_t header, int offset) {
      Entry e;
      e.tag = FIELD;
      e.field.header = header;
      e.field.offset = offset;
      return e;
    }

    static Entry make_stack_field(header_stack_id_t header_stack, int offset) {
      Entry e;
      e.tag = STACK_FIELD;
      e.field.header = header_stack;
      e.field.offset = offset;
      return e;
    }

    static Entry make_lookahead(int offset, int bitwidth) {
      Entry e;
      e.tag = LOOKAHEAD;
      e.lookahead = ParserLookAhead(offset, bitwidth);
      return e;
    }
  };

  std::vector<Entry> entries{};
  std::vector<int> bitwidths{};

  void push_back_field(header_id_t header, int field_offset, int bitwidth) {
    entries.push_back(Entry::make_field(header, field_offset));
    bitwidths.push_back(bitwidth);
  }

  void push_back_stack_field(header_stack_id_t header_stack, int field_offset,
                             int bitwidth) {
    entries.push_back(Entry::make_stack_field(header_stack, field_offset));
    bitwidths.push_back(bitwidth);
  }

  void push_back_lookahead(int offset, int bitwidth) {
    entries.push_back(Entry::make_lookahead(offset, bitwidth));
    bitwidths.push_back(bitwidth);
  }

  std::vector<int> get_bitwidths() const { return bitwidths; }

  void operator()(const PHV &phv, const char *data, ByteContainer *key) const {
    for (const Entry &e : entries) {
      switch (e.tag) {
      case Entry::FIELD:
        key->append(phv.get_field(e.field.header, e.field.offset).get_bytes());
        break;
      case Entry::STACK_FIELD:
        key->append(phv.get_header_stack(e.field.header).get_last()
                    .get_field(e.field.offset).get_bytes());
        break;
      case Entry::LOOKAHEAD:
        e.lookahead.peek(data, key);
        break;
      }
    }
  }
};

class ParseState;

class ParseVSetIface {
 public:
  enum ErrorCode {
    SUCCESS = 0,
    INVALID_PARSE_VSET_NAME,
    ERROR
  };

  virtual ~ParseVSetIface() { }

  virtual void add(const ByteContainer &v) = 0;

  virtual void remove(const ByteContainer &v) = 0;

  virtual bool contains(const ByteContainer &v) const = 0;

  virtual void clear() = 0;

  virtual size_t size() const = 0;
};

// Note that as of today, all parse states using a vset have to be configured
// before any value can be added to a vset (otherwise the value won't be present
// in the shadow copies). This can be easily changed if the need arises in the
// future.
class ParseVSet : public NamedP4Object, public ParseVSetIface {
  template <typename P> friend class ParseSwitchCaseVSet;
  typedef std::unique_lock<std::mutex> Lock;

 public:
  typedef ParseVSetIface::ErrorCode ErrorCode;

  ParseVSet(const std::string &name, p4object_id_t id,
            size_t compressed_bitwidth);

  void add(const ByteContainer &v) override;

  void remove(const ByteContainer &v) override;

  bool contains(const ByteContainer &v) const override;

  void clear() override;

  size_t size() const override;

  size_t get_compressed_bitwidth() const;

 private:
  void add_shadow(ParseVSetIface *shadow);

  size_t compressed_bitwidth;
  // if no mutex we can have inconsistent results accross shadow copies (some
  // have a value, others don't. This mutex is never requested by the dataplane
  // (each shadow has its own).
  mutable std::mutex shadows_mutex{};
  std::vector<ParseVSetIface *> shadows{};
  std::unique_ptr<ParseVSetIface> base{nullptr};
};

class ParseSwitchCaseIface {
 public:
  virtual ~ParseSwitchCaseIface() { }

  virtual bool match(const ByteContainer &input,
                     const ParseState **state) const = 0;

  static std::unique_ptr<ParseSwitchCaseIface>
  make_case(const ByteContainer &key, const ParseState *next_state);

  static std::unique_ptr<ParseSwitchCaseIface>
  make_case_with_mask(const ByteContainer &key, const ByteContainer &mask,
                      const ParseState *next_state);

  static std::unique_ptr<ParseSwitchCaseIface>
  make_case_vset(ParseVSet *vset, std::vector<int> bitwidths,
                 const ParseState *next_state);

  static std::unique_ptr<ParseSwitchCaseIface>
  make_case_vset_with_mask(ParseVSet *vset, const ByteContainer &mask,
                           std::vector<int> bitwidths,
                           const ParseState *next_state);
};

class ParseState : public NamedP4Object {
 public:
  ParseState(const std::string &name, p4object_id_t id);

  void add_extract(header_id_t header);
  void add_extract_to_stack(header_stack_id_t header_stack);

  void add_set_from_field(header_id_t dst_header, int dst_offset,
                          header_id_t src_header, int src_offset);

  void add_set_from_data(header_id_t dst_header, int dst_offset,
                         const Data &src);

  void add_set_from_lookahead(header_id_t dst_header, int dst_offset,
                              int src_offset, int src_bitwidth);

  void add_set_from_expression(header_id_t dst_header, int dst_offset,
                               const ArithExpression &expr);

  void set_key_builder(const ParseSwitchKeyBuilder &builder);

  void add_switch_case(const ByteContainer &key, const ParseState *next_state);
  void add_switch_case(int nbytes_key, const char *key,
                       const ParseState *next_state);

  void add_switch_case_with_mask(const ByteContainer &key,
                                 const ByteContainer &mask,
                                 const ParseState *next_state);
  void add_switch_case_with_mask(int nbytes_key, const char *key,
                                 const char *mask,
                                 const ParseState *next_state);

  void add_switch_case_vset(ParseVSet *vset, const ParseState *next_state);

  void add_switch_case_vset_with_mask(ParseVSet *vset,
                                      const ByteContainer &mask,
                                      const ParseState *next_state);

  void set_default_switch_case(const ParseState *default_next);

  // Copy constructor
  ParseState(const ParseState& other) = delete;

  // Copy assignment operator
  ParseState &operator =(const ParseState& other) = delete;

  // Move constructor
  ParseState(ParseState&& other)= default;

  // Move assignment operator
  ParseState &operator =(ParseState &&other) = default;

  const ParseState *operator()(Packet *pkt, const char *data,
                               size_t *bytes_parsed) const;

 private:
  const ParseState *find_next_state(Packet *pkt, const char *data,
                                    size_t *bytes_parsed) const;

  std::vector<std::unique_ptr<ParserOp> > parser_ops{};
  RegisterSync register_sync{};
  bool has_switch;
  ParseSwitchKeyBuilder key_builder{};
  std::vector<std::unique_ptr<ParseSwitchCaseIface> > parser_switch{};
  const ParseState *default_next_state{nullptr};
};

//! Implements a P4 parser.
class Parser : public NamedP4Object {
 public:
  Parser(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id), init_state(nullptr) { }

  void set_init_state(const ParseState *state) {
    init_state = state;
  }

  //! Extracts Packet headers as specified by the parse graph. When the parser
  //! extracts a header to the PHV, the header is marked as valid. After parsing
  //! a packet, you can send it to the appropriate match-action Pipeline for
  //! processing. Depending on how your target is organized, you could also
  //! send it to another Parser for deeper parsing.
  void parse(Packet *pkt) const;

  //! Deleted copy constructor
  Parser(const Parser &other) = delete;
  //! Deleted copy assignment operator
  Parser &operator=(const Parser &other) = delete;

  //! Default move constructor
  Parser(Parser &&other) /*noexcept*/ = default;
  //! Default move assignment operator
  Parser &operator=(Parser &&other) /*noexcept*/ = default;

 private:
  const ParseState *init_state;
};

}  // namespace bm

#endif  // BM_BM_SIM_PARSER_H_
