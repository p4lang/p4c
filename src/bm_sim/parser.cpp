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

#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/debugger.h>

#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>

#include "extract.h"

namespace bm {

ParserLookAhead::ParserLookAhead(int offset, int bitwidth)
  : byte_offset(offset / 8), bit_offset(offset % 8),
    bitwidth(bitwidth), nbytes((bitwidth + 7) / 8) { }

void
ParserLookAhead::peek(const char *data, ByteContainer *res) const {
  size_t old_size = res->size();
  res->resize(old_size + nbytes);
  char *dst = &(*res)[old_size];
  extract::generic_extract(data + byte_offset, bit_offset, bitwidth, dst);
}

template<>
void
ParserOpSet<field_t>::operator()(Packet *pkt, const char *data,
                                 size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  Field &f_src = phv->get_field(src.header, src.offset);
  f_dst.set(f_src);
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from field ({}, {}) ({})",
    dst.header, dst.offset, src.header, src.offset, f_dst);
}

template<>
void
ParserOpSet<Data>::operator()(Packet *pkt, const char *data,
                              size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  f_dst.set(src);
  BMLOG_DEBUG_PKT(*pkt, "Parser set: setting field ({}, {}) to {}",
                  dst.header, dst.offset, f_dst);
}

template<>
void
ParserOpSet<ParserLookAhead>::operator()(Packet *pkt, const char *data,
                                         size_t *bytes_parsed) const {
  (void) bytes_parsed;
  static thread_local ByteContainer bc;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  int f_bits = f_dst.get_nbits();
  /* I expect the first case to be the most common one. In the first case, we
     extract the packet bytes to the field bytes and sync the bignum value. The
     second case requires extracting the packet bytes to the ByteContainer, then
     importing the bytes into the field's bignum value, then finally exporting
     the bignum value to the field's byte array. I could alternatively write a
     more general extract function which would account for a potential size
     difference between source and destination. */
  // TODO(antonin)
  if (src.bitwidth == f_bits) {
    data += src.byte_offset;
    f_dst.extract(data, src.bit_offset);
  } else {
    bc.clear();
    src.peek(data, &bc);
    f_dst.set(bc);
  }
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from lookahead ({}, {}), "
    "new value is {}",
    dst.header, dst.offset, src.bit_offset, src.bitwidth, f_dst);
}

template<>
void
ParserOpSet<ArithExpression>::operator()(Packet *pkt, const char *data,
                                         size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  src.eval(*phv, &f_dst);
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from expression, new value is {}",
    dst.header, dst.offset, f_dst);
}

template <typename P, bool with_padding = true>
class ParseVSetCommon : public ParseVSetIface {
  typedef std::unique_lock<std::mutex> Lock;

 public:
  explicit ParseVSetCommon(size_t compressed_bitwidth,
                           std::vector<int> bitwidths = {})
      : compressed_bitwidth(compressed_bitwidth),
        bitwidths(std::move(bitwidths)) {
    compute_width();
  }

  void add(const ByteContainer &v) override {
    ByteContainer new_v = do_padding(v);
    transform_input(new_v);
    assert(new_v.size() == width);
    Lock lock(set_mutex);
    set.insert(std::move(new_v));
  }

  void remove(const ByteContainer &v) override {
    ByteContainer new_v = do_padding(v);
    transform_input(new_v);
    assert(new_v.size() == width);
    Lock lock(set_mutex);
    set.erase(new_v);
  }

  bool contains(const ByteContainer &v) const override {
    ByteContainer new_v = v;
    transform_input(new_v);
    assert(v.size() == width);
    Lock lock(set_mutex);
    return set.find(new_v) != set.end();
  }

  void clear() override {
    Lock lock(set_mutex);
    set.clear();
  }

  size_t size() const override {
    Lock lock(set_mutex);
    return set.size();
  }

 protected:
  template <bool wp = with_padding>
  typename std::enable_if<wp, ByteContainer>::type
  do_padding(const ByteContainer &v) {
    if (bitwidths.size() == 1) {
      return v;
    }

    ByteContainer padded_v(width);  // zeros
    int sum_of_widths = 0;
    for (const int w : bitwidths) sum_of_widths += w;
    assert(v.size() == static_cast<size_t>((sum_of_widths + 7) / 8));
    int offset = (v.size() << 3) - sum_of_widths;
    char *dst = padded_v.data();
    const char *src = v.data();
    for (const int w : bitwidths) {
      int w_bytes = (w + 7) / 8;
      extract::generic_extract(src, offset, w, dst);
      dst += w_bytes;
      offset += w;
      src += offset / 8;
      offset = offset % 8;
    }
    return padded_v;
  }

  template <bool wp = with_padding>
  typename std::enable_if<!wp, ByteContainer>::type
  do_padding(const ByteContainer &v) {
    return v;
  }

  // default implementation, does nothing
  void transform_imp(ByteContainer &v) const {  // NOLINT(runtime/references)
    (void) v;
  }

 private:
  void transform_input(ByteContainer &v) const {  // NOLINT(runtime/references)
    static_cast<const P *>(this)->transform_imp(v);
  }

  void compute_width() {
    if (with_padding) {
      assert(bitwidths.size() > 0);
      width = 0;
      for (const int w : bitwidths) width += (w + 7) / 8;
    } else {
      width = (compressed_bitwidth + 7) / 8;
    }
  }

  size_t compressed_bitwidth{};
  size_t width{};
  // TODO(antonin): used shared mutex?
  mutable std::mutex set_mutex{};
  std::unordered_set<ByteContainer, ByteContainerKeyHash> set{};
  std::vector<int> bitwidths{};
};

class ParseVSetBase : public ParseVSetCommon<ParseVSetBase, false> {
 public:
  explicit ParseVSetBase(size_t width)
      : ParseVSetCommon<ParseVSetBase, false>(width) { }
};

class ParseVSetNoMask : public ParseVSetCommon<ParseVSetNoMask> {
 public:
  ParseVSetNoMask(size_t width, std::vector<int> bitwidths)
      : ParseVSetCommon<ParseVSetNoMask>(width, std::move(bitwidths)) { }
};

class ParseVSetMask : public ParseVSetCommon<ParseVSetMask> {
 public:
  ParseVSetMask(size_t width, const ByteContainer &mask,
                std::vector<int> bitwidths)
      : ParseVSetCommon<ParseVSetMask>(width, std::move(bitwidths)),
        mask(mask) { }

  void transform_imp(ByteContainer &v) const {  // NOLINT(runtime/references)
    mask_v(v);
  }

 private:
  void mask_v(ByteContainer &v) const {  // NOLINT(runtime/references)
    for (unsigned int byte_index = 0; byte_index < v.size(); byte_index++)
      v[byte_index] = v[byte_index] & mask[byte_index];
  }

  ByteContainer mask{};
};

ParseVSet::ParseVSet(const std::string &name, p4object_id_t id,
                     size_t compressed_bitwidth)
    : NamedP4Object(name, id), compressed_bitwidth(compressed_bitwidth),
      base(new ParseVSetBase(compressed_bitwidth)) {
  add_shadow(base.get());
}

void
ParseVSet::add_shadow(ParseVSetIface *shadow) {
  shadows.push_back(shadow);
}

void
ParseVSet::add(const ByteContainer &v) {
  Lock lock(shadows_mutex);
  for (auto shadow : shadows) shadow->add(v);
}

void
ParseVSet::remove(const ByteContainer &v) {
  Lock lock(shadows_mutex);
  for (auto shadow : shadows) shadow->remove(v);
}

bool
ParseVSet::contains(const ByteContainer &v) const {
  return base->contains(v);
}

void
ParseVSet::clear() {
  Lock lock(shadows_mutex);
  for (auto shadow : shadows) shadow->clear();
}

size_t
ParseVSet::size() const {
  return base->size();
}

size_t
ParseVSet::get_compressed_bitwidth() const {
  return compressed_bitwidth;
}

class ParseSwitchCase : public ParseSwitchCaseIface {
 public:
  ParseSwitchCase(const ByteContainer &key, const ParseState *next_state)
      : key(key), next_state(next_state) { }

  ParseSwitchCase(int nbytes_key, const char *key, const ParseState *next_state)
      : key(ByteContainer(key, nbytes_key)), next_state(next_state) { }

  bool match(const ByteContainer &input,
             const ParseState **state) const override;

 private:
  ByteContainer key;
  const ParseState *next_state; /* NULL if end */
};

bool
ParseSwitchCase::match(const ByteContainer &input,
                       const ParseState **state) const {
  if (key == input) {
    *state = next_state;
    return true;
  }
  return false;
}

class ParseSwitchCaseWithMask : public ParseSwitchCaseIface {
 public:
  ParseSwitchCaseWithMask(const ByteContainer &key,
                          const ByteContainer &mask,
                          const ParseState *next_state)
      : key(key), mask(mask), next_state(next_state) {
    assert(key.size() == mask.size());
    mask_key();
  }

  ParseSwitchCaseWithMask(int nbytes_key, const char *key, const char *mask,
                          const ParseState *next_state)
      : key(ByteContainer(key, nbytes_key)),
        mask(ByteContainer(mask, nbytes_key)),
        next_state(next_state) {
    mask_key();
  }

  bool match(const ByteContainer &input,
             const ParseState **state) const override;

 private:
  void mask_key();

  ByteContainer key;
  ByteContainer mask{};
  const ParseState *next_state; /* NULL if end */
};

bool
ParseSwitchCaseWithMask::match(const ByteContainer &input,
                               const ParseState **state) const {
  for (unsigned int byte_index = 0; byte_index < key.size(); byte_index++) {
    if (key[byte_index] != (input[byte_index] & mask[byte_index]))
      return false;
  }
  *state = next_state;
  return true;
}

void
ParseSwitchCaseWithMask::mask_key() {
  for (unsigned int byte_index = 0; byte_index < key.size(); byte_index++)
    key[byte_index] = key[byte_index] & mask[byte_index];
}

template <typename P>
class ParseSwitchCaseVSet : public ParseSwitchCaseIface {
 public:
  template <typename ...Args>
  ParseSwitchCaseVSet(ParseVSet *vset, const ParseState *next_state,
                      Args&&...args)
      : vset_shadow(std::forward<Args>(args)...), next_state(next_state) {
    vset->add_shadow(&this->vset_shadow);
  }

  bool match(const ByteContainer &input,
             const ParseState **state) const override {
    if (vset_shadow.contains(input)) {
      *state = next_state;
      return true;
    }
    return false;
  }

 private:
  P vset_shadow;
  const ParseState *next_state; /* NULL if end */
};

class ParseSwitchCaseVSetNoMask : public ParseSwitchCaseVSet<ParseVSetNoMask> {
 public:
  ParseSwitchCaseVSetNoMask(ParseVSet *vset, std::vector<int> bitwidths,
                            const ParseState *next_state)
      : ParseSwitchCaseVSet<ParseVSetNoMask>(
            vset, next_state, vset->get_compressed_bitwidth(),
            std::move(bitwidths)) { }
};

class ParseSwitchCaseVSetWithMask : public ParseSwitchCaseVSet<ParseVSetMask> {
 public:
  ParseSwitchCaseVSetWithMask(ParseVSet *vset, const ByteContainer &mask,
                              std::vector<int> bitwidths,
                              const ParseState *next_state)
      : ParseSwitchCaseVSet<ParseVSetMask>(
            vset, next_state, vset->get_compressed_bitwidth(), mask,
            std::move(bitwidths)) { }
};

std::unique_ptr<ParseSwitchCaseIface>
ParseSwitchCaseIface::make_case(const ByteContainer &key,
                                const ParseState *next_state) {
  return std::unique_ptr<ParseSwitchCaseIface>(new ParseSwitchCase(
      key, next_state));
}

std::unique_ptr<ParseSwitchCaseIface>
ParseSwitchCaseIface::make_case_with_mask(const ByteContainer &key,
                                          const ByteContainer &mask,
                                          const ParseState *next_state) {
  return std::unique_ptr<ParseSwitchCaseIface>(new ParseSwitchCaseWithMask(
      key, mask, next_state));
}

std::unique_ptr<ParseSwitchCaseIface>
ParseSwitchCaseIface::make_case_vset(ParseVSet *vset,
                                     std::vector<int> bitwidths,
                                     const ParseState *next_state) {
  return std::unique_ptr<ParseSwitchCaseIface>(new ParseSwitchCaseVSetNoMask(
      vset, std::move(bitwidths), next_state));
}

std::unique_ptr<ParseSwitchCaseIface>
ParseSwitchCaseIface::make_case_vset_with_mask(ParseVSet *vset,
                                               const ByteContainer &mask,
                                               std::vector<int> bitwidths,
                                               const ParseState *next_state) {
  return std::unique_ptr<ParseSwitchCaseIface>(new ParseSwitchCaseVSetWithMask(
      vset, mask, std::move(bitwidths), next_state));
}

ParseState::ParseState(const std::string &name, p4object_id_t id)
    : NamedP4Object(name, id), has_switch(false) { }

void
ParseState::add_extract(header_id_t header) {
  parser_ops.emplace_back(new ParserOpExtract(header));
}

void
ParseState::add_extract_to_stack(header_stack_id_t header_stack) {
  parser_ops.emplace_back(new ParserOpExtractStack(header_stack));
}

void
ParseState::add_set_from_field(header_id_t dst_header, int dst_offset,
                               header_id_t src_header, int src_offset) {
  parser_ops.emplace_back(new ParserOpSet<field_t>(
      dst_header, dst_offset, field_t::make(src_header, src_offset)));
}

void
ParseState::add_set_from_data(header_id_t dst_header, int dst_offset,
                              const Data &src) {
  parser_ops.emplace_back(
      new ParserOpSet<Data>(dst_header, dst_offset, src));
}

void
ParseState::add_set_from_lookahead(header_id_t dst_header, int dst_offset,
                                   int src_offset, int src_bitwidth) {
  parser_ops.emplace_back(new ParserOpSet<ParserLookAhead>(
      dst_header, dst_offset,
      ParserLookAhead(src_offset, src_bitwidth)));
}

void
ParseState::add_set_from_expression(header_id_t dst_header, int dst_offset,
                                    const ArithExpression &expr) {
  expr.grab_register_accesses(&register_sync);

  parser_ops.emplace_back(
      new ParserOpSet<ArithExpression>(dst_header, dst_offset, expr));
}

void
ParseState::set_key_builder(const ParseSwitchKeyBuilder &builder) {
  has_switch = true;
  key_builder = builder;
}

void
ParseState::add_switch_case(const ByteContainer &key,
                            const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case(key, next_state));
}

void
ParseState::add_switch_case(int nbytes_key, const char *key,
                            const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case(
      ByteContainer(key, nbytes_key), next_state));
}

void
ParseState::add_switch_case_with_mask(const ByteContainer &key,
                                      const ByteContainer &mask,
                                      const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case_with_mask(
      key, mask, next_state));
}

void
ParseState::add_switch_case_with_mask(int nbytes_key, const char *key,
                                      const char *mask,
                                      const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case_with_mask(
      ByteContainer(key, nbytes_key), ByteContainer(mask, nbytes_key),
      next_state));
}

void
ParseState::add_switch_case_vset(ParseVSet *vset,
                                 const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case_vset(
      vset, key_builder.get_bitwidths(), next_state));
}

void
ParseState::add_switch_case_vset_with_mask(ParseVSet *vset,
                                           const ByteContainer &mask,
                                           const ParseState *next_state) {
  parser_switch.push_back(ParseSwitchCaseIface::make_case_vset_with_mask(
      vset, mask, key_builder.get_bitwidths(), next_state));
}

void
ParseState::set_default_switch_case(const ParseState *default_next) {
  default_next_state = default_next;
}

const ParseState *
ParseState::find_next_state(Packet *pkt, const char *data,
                            size_t *bytes_parsed) const {
  // execute parser ops
  PHV *phv = pkt->get_phv();

  {
    RegisterSync::RegisterLocks RL;
    register_sync.lock(&RL);

    for (auto &parser_op : parser_ops)
      (*parser_op)(pkt, data + *bytes_parsed, bytes_parsed);
  }

  if (!has_switch) {
    BMLOG_DEBUG_PKT(
      *pkt,
      "Parser state '{}' has no switch, going to default next state",
      get_name());
    return default_next_state;
  }

  // build key
  static thread_local ByteContainer key;
  key.clear();
  key_builder(*phv, data + *bytes_parsed, &key);

  BMLOG_DEBUG_PKT(*pkt, "Parser state '{}': key is {}",
                  get_name(), key.to_hex());

  // try the matches in order
  const ParseState *next_state = NULL;
  for (const auto &switch_case : parser_switch)
    if (switch_case->match(key, &next_state)) return next_state;

  return default_next_state;
}

const ParseState *
ParseState::operator()(Packet *pkt, const char *data,
                       size_t *bytes_parsed) const {
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_PARSE_STATE | get_id());

  auto next_state = find_next_state(pkt, data, bytes_parsed);

  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_PARSE_STATE) | get_id());

  return next_state;
}

void
Parser::parse(Packet *pkt) const {
  BMELOG(parser_start, *pkt, *this);
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_PARSER | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Parser '{}': start", get_name());
  const char *data = pkt->data();
  if (!init_state) return;
  const ParseState *next_state = init_state;
  size_t bytes_parsed = 0;
  while (next_state) {
    next_state = (*next_state)(pkt, data, &bytes_parsed);
    BMLOG_TRACE("Bytes parsed: {}", bytes_parsed);
  }
  pkt->remove(bytes_parsed);
  BMELOG(parser_done, *pkt, *this);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_PARSER) | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Parser '{}': end", get_name());
}

}  // namespace bm
