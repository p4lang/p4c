#include "bm_sim/deparser.h"

size_t Deparser::get_headers_size(const PHV &phv) const {
  size_t headers_size = 0;
  for(auto it = headers.begin(); it != headers.end(); ++it) {
    const Header &header = phv.get_header(*it);
    if(header.is_valid()){
      headers_size += header.get_nbytes_packet();
    }
  }
  return headers_size;
}

void Deparser::deparse(const PHV &phv, Packet *pkt) const {
  ELOGGER->deparser_start(*pkt, *this);
  char *data = pkt->prepend(get_headers_size(phv));
  int bytes_parsed = 0;
  for(auto it = headers.begin(); it != headers.end(); ++it) {
    const Header &header = phv.get_header(*it);
    if(header.is_valid()){
      ELOGGER->deparser_emit(*pkt, *it);
      header.deparse(data + bytes_parsed);
      bytes_parsed += header.get_nbytes_packet();
    }
  }
  ELOGGER->deparser_done(*pkt, *this);
}
