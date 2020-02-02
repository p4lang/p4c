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
#include <type_traits>

#include <cassert>

#include "phv_forward.h"
#include "named_p4object.h"
#include "stateful.h"
#include "parser_error.h"

namespace bm {

class Packet;

class BoolExpression;
class ArithExpression;
class ActionFn;

class Checksum;

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

  static ParserLookAhead make(int offset, int bitwidth);

  void peek(const char *data, ByteContainer *res) const;
};

static_assert(std::is_pod<ParserLookAhead>::value,
              "ParserLookAhead is used in union and we assume it is POD data");

struct ParserOp {
  virtual ~ParserOp() {}
  virtual void operator()(Packet *pkt, const char *data,
                          size_t *bytes_parsed) const = 0;
};

// need to be in the header because of unit tests
template <typename T>
struct ParserOpSet : ParserOp {
  field_t dst;
  T src;

  ParserOpSet(header_id_t header, int offset, const T &src)
      : dst({header, offset}), src(src) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override;
};

class ParseSwitchKeyBuilder {
 public:
  void push_back_field(header_id_t header, int field_offset, int bitwidth);

  void push_back_stack_field(header_stack_id_t header_stack, int field_offset,
                             int bitwidth);

  void push_back_union_stack_field(header_union_stack_id_t header_union_stack,
                                   size_t header_offset, int field_offset,
                                   int bitwidth);

  void push_back_lookahead(int offset, int bitwidth);

  std::vector<int> get_bitwidths() const;

  void operator()(const PHV &phv, const char *data, ByteContainer *key) const;

 private:
  struct Entry {
    enum {FIELD, LOOKAHEAD, STACK_FIELD, UNION_STACK_FIELD} tag{};
    // I made sure ParserLookAhead was POD data so that it is easy to use in the
    // union
    union {
      field_t field;
      struct {
        header_union_stack_id_t header_union_stack;
        size_t header_offset;
        int offset;
      } union_stack_field;
      ParserLookAhead lookahead;
    };

    static Entry make_field(header_id_t header, int offset);

    static Entry make_stack_field(header_stack_id_t header_stack, int offset);

    static Entry make_union_stack_field(
        header_union_stack_id_t header_union_stack, size_t header_offset,
        int offset);

    static Entry make_lookahead(int offset, int bitwidth);
  };

  std::vector<Entry> entries{};
  std::vector<int> bitwidths{};
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

class ParseVSetBase;  // forward declaration

// Note that as of today, all parse states using a vset have to be configured
// before any value can be added to a vset (otherwise the value won't be present
// in the shadow copies). This can be easily changed if the need arises in the
// future.
class ParseVSet : public NamedP4Object, public ParseVSetIface {
  template <typename P> friend class ParseSwitchCaseVSet;
  using Lock = std::unique_lock<std::mutex>;

 public:
  using ErrorCode = ParseVSetIface::ErrorCode;

  ParseVSet(const std::string &name, p4object_id_t id,
            size_t compressed_bitwidth);

  ~ParseVSet();

  void add(const ByteContainer &v) override;

  void remove(const ByteContainer &v) override;

  bool contains(const ByteContainer &v) const override;

  void clear() override;

  size_t size() const override;

  size_t get_compressed_bitwidth() const;

  std::vector<ByteContainer> get() const;

 private:
  void add_shadow(ParseVSetIface *shadow);

  size_t compressed_bitwidth;
  // if no mutex we can have inconsistent results accross shadow copies (some
  // have a value, others don't. This mutex is never requested by the dataplane
  // (each shadow has its own).
  mutable std::mutex shadows_mutex{};
  std::vector<ParseVSetIface *> shadows{};
  std::unique_ptr<ParseVSetBase> base;
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
  void add_extract_VL(header_id_t header,
                      const ArithExpression &field_length_expr,
                      size_t max_header_bytes);
  void add_extract_to_stack(header_stack_id_t header_stack);
  void add_extract_to_stack_VL(header_stack_id_t header_stack,
                               const ArithExpression &field_length_expr,
                               size_t max_header_bytes);
  void add_extract_to_union_stack(header_union_stack_id_t header_union_stack,
                                  size_t header_offset);
  void add_extract_to_union_stack_VL(header_union_stack_id_t header_union_stack,
                                     size_t header_offset,
                                     const ArithExpression &field_length_expr,
                                     size_t max_header_bytes);

  void add_set_from_field(header_id_t dst_header, int dst_offset,
                          header_id_t src_header, int src_offset);

  void add_set_from_data(header_id_t dst_header, int dst_offset,
                         const Data &src);

  void add_set_from_lookahead(header_id_t dst_header, int dst_offset,
                              int src_offset, int src_bitwidth);

  void add_set_from_expression(header_id_t dst_header, int dst_offset,
                               const ArithExpression &expr);

  void add_verify(const BoolExpression &condition,
                  const ArithExpression &error_expr);

  void add_method_call(ActionFn *action_fn);

  void add_shift(size_t shift_bytes);

  void add_advance_from_data(const Data &shift_bits);
  void add_advance_from_expression(const ArithExpression &shift_bits);
  void add_advance_from_field(header_id_t shift_header, int shift_offset);

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

  int expected_switch_case_key_size() const;

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
  Parser(const std::string &name, p4object_id_t id,
         const ErrorCodeMap *error_codes);

  void set_init_state(const ParseState *state);

  void add_checksum(const Checksum *checksum);

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
  void verify_checksums(Packet *pkt) const;

  const ParseState *init_state;
  const ErrorCodeMap *error_codes;
  const ErrorCode no_error;
  std::vector<const Checksum *> checksums{};
};

}  // namespace bm

#endif  // BM_BM_SIM_PARSER_H_
