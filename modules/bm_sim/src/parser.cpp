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

const ParseState *ParseState::operator()(Packet *pkt, const char *data,
					 size_t *bytes_parsed) const{
  // execute parser ops
  ParserOp *parser_op;
  PHV *phv = pkt->get_phv();
  for (std::vector<ParserOp *>::const_iterator it = parser_ops.begin();
       it != parser_ops.end();
       ++it) {
    parser_op = *it;
    (*parser_op)(pkt, data + *bytes_parsed, bytes_parsed);
  }

  if(!has_switch) return NULL;

  // build key
  static thread_local ByteContainer key;
  key.clear();
  key_builder(*phv, data, key);

  // try the matches in order
  const ParseState *next_state = NULL;
  for (std::vector<ParseSwitchCase>::const_iterator it = parser_switch.begin();
       it != parser_switch.end();
       ++it) {
    if(it->match(key, &next_state)) return next_state;
  }

  return default_next_state;
}

void Parser::parse(Packet *pkt) const {
  ELOGGER->parser_start(*pkt, *this);
  const char *data = pkt->data();
  if(!init_state) return;
  const ParseState *next_state = init_state;
  size_t bytes_parsed = 0;
  while(next_state) {
    next_state = (*next_state)(pkt, data, &bytes_parsed);
    // std::cout << "bytes parsed: " << bytes_parsed << std::endl;
  }
  pkt->remove(bytes_parsed);
  ELOGGER->parser_done(*pkt, *this);
}
