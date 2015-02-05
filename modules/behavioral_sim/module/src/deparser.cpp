#include "deparser.h"

void Deparser::deparse(const PHV &phv, char *data) const {
  int bytes_parsed = 0;
  for(vector<header_id_t>::const_iterator it = headers.begin();
      it != headers.end();
      ++it) {
    const Header &header = phv.get_header(*it);
    if(header.is_valid()){
      header.deparse(data + bytes_parsed);
      bytes_parsed += header.get_nbytes_packet();
    }
  }
}
