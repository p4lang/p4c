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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/debugger.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/expressions.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/checksums.h>
#include <bm/bm_sim/core/primitives.h>

#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>

#include "extract.h"

namespace bm {

ParserLookAhead
ParserLookAhead::make(int offset, int bitwidth) {
  return {offset / 8, offset % 8, bitwidth,
          static_cast<size_t>((bitwidth + 7) / 8)};
}

void
ParserLookAhead::peek(const char *data, ByteContainer *res) const {
  auto old_size = res->size();
  res->resize(old_size + nbytes);
  char *dst = &(*res)[old_size];
  extract::generic_extract(data + byte_offset, bit_offset, bitwidth, dst);
}

template<>
void
ParserOpSet<field_t>::operator()(Packet *pkt, const char *data,
                                 size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  auto phv = pkt->get_phv();
  auto &f_dst = phv->get_field(dst.header, dst.offset);
  auto &f_src = phv->get_field(src.header, src.offset);
  if (f_dst.is_VL()) {
    assert(f_src.is_VL());
    core::assign_VL()(f_dst, f_src);
  } else {
    core::assign()(f_dst, f_src);
  }
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field '{}' from field '{}' ({})",
    phv->get_field_name(dst.header, dst.offset),
    phv->get_field_name(src.header, src.offset),
    f_dst);
}

template<>
void
ParserOpSet<Data>::operator()(Packet *pkt, const char *data,
                              size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  auto phv = pkt->get_phv();
  auto &f_dst = phv->get_field(dst.header, dst.offset);
  core::assign()(f_dst, src);
  BMLOG_DEBUG_PKT(*pkt, "Parser set: setting field '{}' to {}",
                  phv->get_field_name(dst.header, dst.offset), f_dst);
}

template<>
void
ParserOpSet<ParserLookAhead>::operator()(Packet *pkt, const char *data,
                                         size_t *bytes_parsed) const {
  static thread_local ByteContainer bc;
  auto phv = pkt->get_phv();
  if (pkt->get_data_size() - *bytes_parsed < src.nbytes)
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
  auto &f_dst = phv->get_field(dst.header, dst.offset);
  auto f_bits = f_dst.get_nbits();
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
    "Parser set: setting field '{}' from lookahead ({}, {}), new value is {}",
    phv->get_field_name(dst.header, dst.offset), src.bit_offset, src.bitwidth,
    f_dst);
}

template<>
void
ParserOpSet<ArithExpression>::operator()(Packet *pkt, const char *data,
                                         size_t *bytes_parsed) const {
  (void) bytes_parsed; (void) data;
  auto phv = pkt->get_phv();
  auto &f_dst = phv->get_field(dst.header, dst.offset);
  src.eval(*phv, &f_dst);
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field '{}' from expression, new value is {}",
    phv->get_field_name(dst.header, dst.offset), f_dst);
}

ParseSwitchKeyBuilder::Entry
ParseSwitchKeyBuilder::Entry::make_field(header_id_t header, int offset) {
  Entry e;
  e.tag = FIELD;
  e.field = field_t::make(header, offset);
  return e;
}

ParseSwitchKeyBuilder::Entry
ParseSwitchKeyBuilder::Entry::make_stack_field(header_stack_id_t header_stack,
                                               int offset) {
  Entry e;
  e.tag = STACK_FIELD;
  e.field.header = header_stack;
  e.field.offset = offset;
  return e;
}

ParseSwitchKeyBuilder::Entry
ParseSwitchKeyBuilder::Entry::make_union_stack_field(
    header_union_stack_id_t header_union_stack, size_t header_offset,
    int offset) {
  Entry e;
  e.tag = UNION_STACK_FIELD;
  e.union_stack_field.header_union_stack = header_union_stack;
  e.union_stack_field.header_offset = header_offset;
  e.union_stack_field.offset = offset;
  return e;
}

ParseSwitchKeyBuilder::Entry
ParseSwitchKeyBuilder::Entry::make_lookahead(int offset, int bitwidth) {
  Entry e;
  e.tag = LOOKAHEAD;
  e.lookahead = ParserLookAhead::make(offset, bitwidth);
  return e;
}

void
ParseSwitchKeyBuilder::push_back_field(header_id_t header, int field_offset,
                                       int bitwidth) {
  entries.push_back(Entry::make_field(header, field_offset));
  bitwidths.push_back(bitwidth);
}

void
ParseSwitchKeyBuilder::push_back_stack_field(header_stack_id_t header_stack,
                                             int field_offset, int bitwidth) {
  entries.push_back(Entry::make_stack_field(header_stack, field_offset));
  bitwidths.push_back(bitwidth);
}

void
ParseSwitchKeyBuilder::push_back_union_stack_field(
    header_union_stack_id_t header_union_stack, size_t header_offset,
    int field_offset, int bitwidth) {
  entries.push_back(Entry::make_union_stack_field(
      header_union_stack, header_offset, field_offset));
  bitwidths.push_back(bitwidth);
}

void
ParseSwitchKeyBuilder::push_back_lookahead(int offset, int bitwidth) {
  entries.push_back(Entry::make_lookahead(offset, bitwidth));
  bitwidths.push_back(bitwidth);
}

std::vector<int>
ParseSwitchKeyBuilder::get_bitwidths() const {
  return bitwidths;
}

void
ParseSwitchKeyBuilder::operator()(const PHV &phv, const char *data,
                                  ByteContainer *key) const {
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
      case Entry::UNION_STACK_FIELD:
        key->append(
            phv.get_header_union_stack(
                e.union_stack_field.header_union_stack).get_last()
            .at(e.union_stack_field.header_offset)
            .get_field(e.union_stack_field.offset).get_bytes());
        break;
    }
  }
}

namespace {

void check_enough_data_for_extract(const Packet &pkt, size_t bytes_parsed,
                                   const Header &hdr) {
  if (pkt.get_data_size() - bytes_parsed <
      static_cast<size_t>(hdr.get_nbytes_packet()))
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
}

void extract_fixed(Header *hdr,
                   Packet *pkt, const char *data, size_t *bytes_parsed) {
  auto phv = pkt->get_phv();
  BMELOG(parser_extract, *pkt, hdr->get_id());
  check_enough_data_for_extract(*pkt, *bytes_parsed, *hdr);
  hdr->extract(data, *phv);
  *bytes_parsed += hdr->get_nbytes_packet();
}

void extract_VL(Header *hdr,
                const ArithExpression &VL_expr, size_t max_header_bytes,
                Packet *pkt, const char *data, size_t *bytes_parsed) {
  static thread_local Data computed_nbits;
  auto phv = pkt->get_phv();
  BMELOG(parser_extract, *pkt, hdr->get_id());

  VL_expr.eval(*phv, &computed_nbits);
  auto nbits = computed_nbits.get<int>();
  if ((nbits % 8) != 0) {
      BMLOG_ERROR_PKT(*pkt, "VL field bitwidth {} needs to be a multiple of 8",
                      nbits);
      throw parser_exception_core(ErrorCodeMap::Core::ParserInvalidArgument);
  }
  // get_nbytes_packet counts the VL field in the header as 0 bits
  auto bytes_to_extract = static_cast<size_t>(
      hdr->get_nbytes_packet() + nbits / 8);
  if (pkt->get_ingress_length() - *bytes_parsed < bytes_to_extract)
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);

  if (max_header_bytes != 0 && max_header_bytes < bytes_to_extract)
    throw parser_exception_core(ErrorCodeMap::Core::HeaderTooShort);

  hdr->extract_VL(data, nbits);
  // get_nbytes_packet now returns a value that includes the VL field bitwidth
  *bytes_parsed += hdr->get_nbytes_packet();
}

}  // namespace

struct ParserOpExtract : ParserOp {
  header_id_t header;

  explicit ParserOpExtract(header_id_t header)
    : header(header) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    auto phv = pkt->get_phv();
    auto &hdr = phv->get_header(header);
    BMLOG_DEBUG_PKT(*pkt, "Extracting header '{}'", hdr.get_name());
    extract_fixed(&hdr, pkt, data, bytes_parsed);
  }
};

// TODO(antonin): this could be merged with ParserOpExtract as we did for the
// stack extracts below.
struct ParserOpExtractVL : ParserOp {
  header_id_t header;
  ArithExpression field_length_expr;
  size_t max_header_bytes;

  explicit ParserOpExtractVL(header_id_t header,
                             const ArithExpression &field_length_expr,
                             size_t max_header_bytes)
      : header(header), field_length_expr(field_length_expr),
        max_header_bytes(max_header_bytes) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    auto phv = pkt->get_phv();
    auto &hdr = phv->get_header(header);
    BMLOG_DEBUG_PKT(*pkt, "Extracting variable-sized header '{}'",
                    hdr.get_name());
    extract_VL(&hdr, field_length_expr, max_header_bytes,
               pkt, data, bytes_parsed);
  }
};

// push back a header on a tag stack
struct ParserOpExtractStack : ParserOp {
  header_stack_id_t header_stack;
  ArithExpression field_length_expr;
  // varbit<0> is not allowed in P4_16; we therefore use max_header_bytes > 0
  // as a signal that extract_VL was used.
  size_t max_header_bytes;

  explicit ParserOpExtractStack(header_stack_id_t header_stack)
      : header_stack(header_stack), max_header_bytes(0) { }

  ParserOpExtractStack(header_stack_id_t header_stack,
                       const ArithExpression &field_length_expr,
                       size_t max_header_bytes)
      : header_stack(header_stack), field_length_expr(field_length_expr),
        max_header_bytes(max_header_bytes) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    auto phv = pkt->get_phv();
    auto &stack = phv->get_header_stack(header_stack);
    if (stack.is_full())
      throw parser_exception_core(ErrorCodeMap::Core::StackOutOfBounds);
    auto &next_hdr = stack.get_next();
    BMLOG_DEBUG_PKT(*pkt, "Extracting to header stack {}, next header is {}",
                    header_stack, next_hdr.get_id());
    if (max_header_bytes == 0) {
      extract_fixed(&next_hdr, pkt, data, bytes_parsed);
    } else {
      extract_VL(&next_hdr, field_length_expr, max_header_bytes,
                 pkt, data, bytes_parsed);
    }
    stack.push_back();
  }
};

// TODO(antonin): reuse some code from ParserOpExtractStack using the stack
// common interface?
struct ParserOpExtractUnionStack : ParserOp {
  header_union_stack_id_t header_union_stack;
  size_t header_offset;
  ArithExpression field_length_expr;
  // varbit<0> is not allowed in P4_16; we therefore use max_header_bytes > 0
  // as a signal that extract_VL was used.
  size_t max_header_bytes;

  explicit ParserOpExtractUnionStack(header_union_stack_id_t header_union_stack,
                                     size_t header_offset)
      : header_union_stack(header_union_stack), header_offset(header_offset),
        max_header_bytes(0) { }

  ParserOpExtractUnionStack(header_union_stack_id_t header_union_stack,
                            size_t header_offset,
                            const ArithExpression &field_length_expr,
                            size_t max_header_bytes)
      : header_union_stack(header_union_stack), header_offset(header_offset),
        field_length_expr(field_length_expr),
        max_header_bytes(max_header_bytes) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    auto phv = pkt->get_phv();
    auto &union_stack = phv->get_header_union_stack(header_union_stack);
    if (union_stack.is_full())
      throw parser_exception_core(ErrorCodeMap::Core::StackOutOfBounds);
    auto &next_union = union_stack.get_next();
    auto &next_hdr = next_union.at(header_offset);
    BMLOG_DEBUG_PKT(*pkt,
                    "Extracting to header union stack {}, next header is {}",
                    header_union_stack, next_hdr.get_id());
    if (max_header_bytes == 0) {
      extract_fixed(&next_hdr, pkt, data, bytes_parsed);
    } else {
      extract_VL(&next_hdr, field_length_expr, max_header_bytes,
                 pkt, data, bytes_parsed);
    }
    union_stack.push_back();
  }
};

struct ParserOpVerify : ParserOp {
  BoolExpression condition;
  ArithExpression error_expr;

  ParserOpVerify(const BoolExpression &condition,
                 const ArithExpression &error_expr)
      : condition(condition), error_expr(error_expr) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    (void) data; (void) bytes_parsed;
    static thread_local Data error;
    const auto &phv = *pkt->get_phv();
    if (!condition.eval(phv)) {
      error_expr.eval(phv, &error);
      throw parser_exception_arch(ErrorCode(error.get<ErrorCode::type_t>()));
    }
  }
};

struct ParserOpMethodCall : ParserOp {
  ActionFnEntry action;

  explicit ParserOpMethodCall(ActionFn *action_fn)
      : action(action_fn) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    (void) data; (void) bytes_parsed;
    BMLOG_DEBUG_PKT(*pkt, "Executing method {}",
                    action.get_action_fn()->get_name());
    action.execute(pkt);
  }
};

struct ParserOpShift : ParserOp {
  size_t shift_bytes;

  explicit ParserOpShift(size_t shift_bytes)
      : shift_bytes(shift_bytes) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override {
    (void) data;
    if (pkt->get_ingress_length() - *bytes_parsed < shift_bytes)
      throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
    *bytes_parsed += shift_bytes;
  }
};

template <typename T>
struct ParserOpAdvance : ParserOp {
  T shift_bits;

  explicit ParserOpAdvance(const T& shift_bits)
      : shift_bits(shift_bits) { }

  void operator()(Packet *pkt, const char *data,
                  size_t *bytes_parsed) const override;
};

template<>
void
ParserOpAdvance<Data>::operator()(Packet *pkt, const char *data,
                                  size_t *bytes_parsed) const {
  (void) data;
  // The JSON should never specify a negative constant as the shift amount.
  assert(shift_bits.sign() >= 0);
  const auto shift_bits_uint = shift_bits.get<size_t>();
  if ((shift_bits_uint % 8) != 0) {
      BMLOG_ERROR_PKT(*pkt, "advance bits amount {} must be a multiple of 8",
                      shift_bits_uint);
      throw parser_exception_core(ErrorCodeMap::Core::ParserInvalidArgument);
  }
  const auto shift_bytes_uint = shift_bits_uint / 8;
  BMLOG_DEBUG_PKT(*pkt, "Advancing by {} bytes", shift_bytes_uint);
  if (pkt->get_ingress_length() - *bytes_parsed < shift_bytes_uint)
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
  *bytes_parsed += shift_bytes_uint;
}

template<>
void
ParserOpAdvance<ArithExpression>::operator()(Packet *pkt, const char *data,
                                             size_t *bytes_parsed) const {
  (void) data;
  static thread_local Data shift_bits_data;
  auto phv = pkt->get_phv();
  shift_bits.eval(*phv, &shift_bits_data);
  // For a JSON file generated by a P4_16 compiler, the expression should never
  // evaluate to a negative value, because the shift amount is of type bit<32>.
  assert(shift_bits_data.sign() >= 0);
  const auto shift_bits_uint = shift_bits_data.get<size_t>();
  if ((shift_bits_uint % 8) != 0) {
      BMLOG_ERROR_PKT(*pkt, "advance bits amount {} must be a multiple of 8",
                      shift_bits_uint);
      throw parser_exception_core(ErrorCodeMap::Core::ParserInvalidArgument);
  }
  const auto shift_bytes_uint = shift_bits_uint / 8;
  BMLOG_DEBUG_PKT(*pkt, "Advancing by {} bytes", shift_bytes_uint);
  if (pkt->get_ingress_length() - *bytes_parsed < shift_bytes_uint)
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
  *bytes_parsed += shift_bytes_uint;
}

template<>
void
ParserOpAdvance<field_t>::operator()(Packet *pkt, const char *data,
                                     size_t *bytes_parsed) const {
  (void) data;
  auto phv = pkt->get_phv();
  auto &f_shift = phv->get_field(shift_bits.header, shift_bits.offset);
  // For a JSON file generated by a P4_16 compiler, the field should never
  // evaluate to a negative value, because the shift amount is of type bit<32>.
  assert(f_shift.sign() >= 0);
  const auto shift_bits_uint = f_shift.get<size_t>();
  if ((shift_bits_uint % 8) != 0) {
      BMLOG_ERROR_PKT(*pkt, "advance bits amount {} must be a multiple of 8",
                      shift_bits_uint);
      throw parser_exception_core(ErrorCodeMap::Core::ParserInvalidArgument);
  }
  const auto shift_bytes_uint = shift_bits_uint / 8;
  BMLOG_DEBUG_PKT(*pkt, "Advancing by {} bytes", shift_bytes_uint);
  if (pkt->get_ingress_length() - *bytes_parsed < shift_bytes_uint)
    throw parser_exception_core(ErrorCodeMap::Core::PacketTooShort);
  *bytes_parsed += shift_bytes_uint;
}

template <typename P, bool with_padding = true>
class ParseVSetCommon : public ParseVSetIface {
  using Lock = std::unique_lock<std::mutex>;

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

  std::vector<ByteContainer> get() const {
    Lock lock(set_mutex);
    return std::vector<ByteContainer>(set.begin(), set.end());
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

  std::vector<ByteContainer> get() const {
    return ParseVSetCommon<ParseVSetBase, false>::get();
  }
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

ParseVSet::~ParseVSet() = default;

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

std::vector<ByteContainer>
ParseVSet::get() const {
  // only the base has a get() method, as it stores values unmodified
  return base->get();
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
ParseState::add_extract_VL(header_id_t header,
                           const ArithExpression &field_length_expr,
                           size_t max_header_bytes) {
  parser_ops.emplace_back(new ParserOpExtractVL(
      header, field_length_expr, max_header_bytes));
}

void
ParseState::add_extract_to_stack(header_stack_id_t header_stack) {
  parser_ops.emplace_back(new ParserOpExtractStack(header_stack));
}

void
ParseState::add_extract_to_stack_VL(header_stack_id_t header_stack,
                                    const ArithExpression &field_length_expr,
                                    size_t max_header_bytes) {
  parser_ops.emplace_back(new ParserOpExtractStack(
      header_stack, field_length_expr, max_header_bytes));
}

void
ParseState::add_extract_to_union_stack(
    header_union_stack_id_t header_union_stack, size_t header_offset) {
  parser_ops.emplace_back(new ParserOpExtractUnionStack(
      header_union_stack, header_offset));
}

void
ParseState::add_extract_to_union_stack_VL(
    header_union_stack_id_t header_union_stack, size_t header_offset,
    const ArithExpression &field_length_expr, size_t max_header_bytes) {
  parser_ops.emplace_back(new ParserOpExtractUnionStack(
      header_union_stack, header_offset, field_length_expr, max_header_bytes));
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
      ParserLookAhead::make(src_offset, src_bitwidth)));
}

void
ParseState::add_set_from_expression(header_id_t dst_header, int dst_offset,
                                    const ArithExpression &expr) {
  expr.grab_register_accesses(&register_sync);

  parser_ops.emplace_back(
      new ParserOpSet<ArithExpression>(dst_header, dst_offset, expr));
}

void
ParseState::add_advance_from_data(const Data &shift_bytes) {
  parser_ops.emplace_back(new ParserOpAdvance<Data>(shift_bytes));
}

void
ParseState::add_advance_from_expression(const ArithExpression &shift_bytes) {
  parser_ops.emplace_back(new ParserOpAdvance<ArithExpression>(shift_bytes));
}

void
ParseState::add_advance_from_field(header_id_t shift_header, int shift_offset) {
  parser_ops.emplace_back(new ParserOpAdvance<field_t>(
      field_t::make(shift_header, shift_offset)));
}

void
ParseState::add_verify(const BoolExpression &condition,
                       const ArithExpression &error_expr) {
  parser_ops.emplace_back(new ParserOpVerify(condition, error_expr));
}

void
ParseState::add_method_call(ActionFn *action_fn) {
  action_fn->grab_register_accesses(&register_sync);
  parser_ops.emplace_back(new ParserOpMethodCall(action_fn));
}

void
ParseState::add_shift(size_t shift_bytes) {
  parser_ops.emplace_back(new ParserOpShift(shift_bytes));
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

int
ParseState::expected_switch_case_key_size() const {
  int size = 0;
  for (auto bitwidth : key_builder.get_bitwidths())
    size += (bitwidth + 7) / 8;
  return size;
}

const ParseState *
ParseState::find_next_state(Packet *pkt, const char *data,
                            size_t *bytes_parsed) const {
  // execute parser ops
  auto phv = pkt->get_phv();

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
  const ParseState *next_state = nullptr;
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

Parser::Parser(const std::string &name, p4object_id_t id,
               const ErrorCodeMap *error_codes)
    : NamedP4Object(name, id), init_state(nullptr), error_codes(error_codes),
      no_error(error_codes->from_core(ErrorCodeMap::Core::NoError)) { }

void
Parser::set_init_state(const ParseState *state) {
  init_state = state;
}

void
Parser::add_checksum(const Checksum *checksum) {
  checksums.push_back(checksum);
}

void
Parser::verify_checksums(Packet *pkt) const {
  for (auto checksum : checksums) {
    auto is_correct = checksum->verify(*pkt);
    if (!is_correct) {
      pkt->set_checksum_error(true);
      BMLOG_ERROR_PKT(*pkt, "Checksum '{}' is not correct",
                      checksum->get_name());
    }
  }
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
  // at the beginning of parsing, we "reset" the error code to
  // Core::NoError and the checksum_error to false
  pkt->set_error_code(no_error);
  pkt->set_checksum_error(false);
  const char *data = pkt->data();
  if (!init_state) return;
  const ParseState *next_state = init_state;
  size_t bytes_parsed = 0;
  while (next_state) {
    BMLOG_DEBUG_PKT(*pkt, "Parser '{}' entering state '{}'",
                    get_name(), next_state->get_name());
    try {
      next_state = (*next_state)(pkt, data, &bytes_parsed);
    } catch (const parser_exception &e) {
      auto error_code = e.get(*error_codes);
      BMLOG_ERROR_PKT(*pkt, "Exception while parsing: {}",
                      error_codes->to_name(error_code));
      pkt->set_error_code(error_code);
      break;
    }
    BMLOG_TRACE_PKT(*pkt, "Bytes parsed: {}", bytes_parsed);
  }
  pkt->remove(bytes_parsed);
  verify_checksums(pkt);
  BMELOG(parser_done, *pkt, *this);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_PARSER) | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Parser '{}': end", get_name());
}

}  // namespace bm
