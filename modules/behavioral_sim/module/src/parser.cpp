#include "parser.h"

bool ParseSwitchCase::match(const ByteContainer &input,
			    const ParseState **state) const {
  if(!with_mask) {
    if(key == input) {
      *state = next_state;
      return true;
    }
  }
  else {
    int byte_index;
    for(byte_index = 0; byte_index < nbytes_key; byte_index++) {
      if(key[byte_index] != (input[byte_index] & mask[byte_index]))
	return false;
    }
    *state = next_state;
    return true;
  }
  return false;
}

const ParseState *ParseState::operator()(const char *data,
					 PHV &phv, int *bytes_parsed) const{
  // execute parser ops
  ParserOp *parser_op;
  for (std::vector<ParserOp *>::const_iterator it = parser_ops.begin();
       it != parser_ops.end();
       ++it) {
    parser_op = *it;
    (*parser_op)(data, phv, bytes_parsed);
  }

  if(!has_switch) return NULL;

  // build key
  ByteContainer key;
  key_builder(phv, data, key);

  // try the matches in order
  const ParseState *next_state = NULL;
  for (std::vector<ParseSwitchCase>::const_iterator it = parser_switch.begin();
       it != parser_switch.end();
       ++it) {
    if(it->match(key, &next_state)) return next_state;
  }

  // not match, return NULL (EOP)
  return NULL;
}

void Parser::parse(const char *data, PHV &phv) const {
  if(!init_state) return;
  const ParseState *next_state = init_state;
  int bytes_parsed = 0;
  while(next_state) {
    next_state = (*next_state)(data + bytes_parsed, phv, &bytes_parsed);
    // std::cout << "bytes parsed: " << bytes_parsed << std::endl;
  }
}
