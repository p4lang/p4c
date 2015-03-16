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

const ParseState *ParseState::operator()(const Packet &pkt, const char *data,
					 PHV *phv, size_t *bytes_parsed) const{
  // execute parser ops
  ParserOp *parser_op;
  for (std::vector<ParserOp *>::const_iterator it = parser_ops.begin();
       it != parser_ops.end();
       ++it) {
    parser_op = *it;
    (*parser_op)(pkt, data, phv, bytes_parsed);
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

void Parser::parse(Packet *pkt, PHV *phv) const {
  ELOGGER->parser_start(*pkt, *this);
  const char *data = pkt->data();
  if(!init_state) return;
  const ParseState *next_state = init_state;
  size_t bytes_parsed = 0;
  while(next_state) {
    next_state = (*next_state)(*pkt, data + bytes_parsed, phv, &bytes_parsed);
    // std::cout << "bytes parsed: " << bytes_parsed << std::endl;
  }
  pkt->remove(bytes_parsed);
  ELOGGER->parser_done(*pkt, *this);
}
