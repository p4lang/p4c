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

#include "bm_sim/parser.h"
#include "extract.h"

ParserLookAhead::ParserLookAhead(int offset, int bitwidth)
  : byte_offset(offset / 8), bit_offset(offset % 8),
    bitwidth(bitwidth), nbytes((bitwidth + 7) / 8) { }

void ParserLookAhead::peek(const char *data, ByteContainer &res) const {
  size_t old_size = res.size();
  res.resize(old_size + nbytes);
  char *dst = &res[old_size];
  generic_extract(data + byte_offset, bit_offset, bitwidth, dst);
}

template<>
void
ParserOpSet<field_t>::operator()(
  Packet *pkt, const char *data, size_t *bytes_parsed
) const
{
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  Field &f_src = phv->get_field(src.header, src.offset);
  f_dst.set(f_src);
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from field ({}, {}) ({})",
    dst.header, dst.offset, src.header, src.offset, f_dst
  );
}

template<>
void
ParserOpSet<Data>::operator()(
  Packet *pkt, const char *data, size_t *bytes_parsed
) const
{
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  f_dst.set(src);
  BMLOG_DEBUG_PKT(*pkt, "Parser set: setting field ({}, {}) to {}",
		  dst.header, dst.offset, f_dst);
}

template<>
void
ParserOpSet<ParserLookAhead>::operator()(
  Packet *pkt, const char *data, size_t *bytes_parsed
) const
{
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
  // TODO
  if(src.bitwidth == f_bits) {
    data += src.byte_offset;
    f_dst.extract(data, src.bit_offset);
  }
  else {
    bc.clear();
    src.peek(data, bc);
    f_dst.set(bc);
  }
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from lookahead ({}, {}), new value is {}",
    dst.header, dst.offset, src.bit_offset, src.bitwidth, f_dst
  );
}

// TODO: use pointer instead for ArithExpression?
template<>
void
ParserOpSet<ArithExpression>::operator()(
  Packet *pkt, const char *data, size_t *bytes_parsed
) const
{
  (void) bytes_parsed; (void) data;
  PHV *phv = pkt->get_phv();
  Field &f_dst = phv->get_field(dst.header, dst.offset);
  src.eval(*phv, &f_dst);
  BMLOG_DEBUG_PKT(
    *pkt,
    "Parser set: setting field ({}, {}) from expression, new value is {}",
    dst.header, dst.offset, f_dst
  );
}

bool ParseSwitchCase::match(const ByteContainer &input,
			    const ParseState **state) const {
  if(!with_mask) {
    if(key == input) {
      *state = next_state;
      return true;
    }
  }
  else {
    for(unsigned int byte_index = 0; byte_index < key.size(); byte_index++) {
      if(key[byte_index] != (input[byte_index] & mask[byte_index]))
	return false;
    }
    *state = next_state;
    return true;
  }
  return false;
}

void ParseSwitchCase::mask_key() {
  for(unsigned int byte_index = 0; byte_index < key.size(); byte_index++)
    key[byte_index] = key[byte_index] & mask[byte_index];
}


const ParseState *ParseState::operator()(Packet *pkt, const char *data,
					 size_t *bytes_parsed) const{
  // execute parser ops
  PHV *phv = pkt->get_phv();
  for (auto &parser_op : parser_ops)
    (*parser_op)(pkt, data + *bytes_parsed, bytes_parsed);

  if(!has_switch) {
    BMLOG_DEBUG_PKT(
      *pkt,
      "Parser state '{}' has no switch, going to default next state",
      get_name()
    );
    return default_next_state;
  }

  // build key
  static thread_local ByteContainer key;
  key.clear();
  key_builder(*phv, data + *bytes_parsed, key);

  BMLOG_DEBUG_PKT(*pkt, "Parser state '{}': key is {}",
		  get_name(), key.to_hex());

  // try the matches in order
  const ParseState *next_state = NULL;
  for (const auto &switch_case : parser_switch)
    if(switch_case.match(key, &next_state)) return next_state;

  return default_next_state;
}

void Parser::parse(Packet *pkt) const {
  ELOGGER->parser_start(*pkt, *this);
  BMLOG_DEBUG_PKT(*pkt, "Parser '{}': start", get_name());
  const char *data = pkt->data();
  if(!init_state) return;
  const ParseState *next_state = init_state;
  size_t bytes_parsed = 0;
  while(next_state) {
    next_state = (*next_state)(pkt, data, &bytes_parsed);
    BMLOG_TRACE("Bytes parsed: {}", bytes_parsed);
  }
  pkt->remove(bytes_parsed);
  ELOGGER->parser_done(*pkt, *this);
  BMLOG_DEBUG_PKT(*pkt, "Parser '{}': end", get_name());
}
