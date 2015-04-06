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

void Deparser::deparse(Packet *pkt) const {
  PHV *phv = pkt->get_phv();
  ELOGGER->deparser_start(*pkt, *this);
  update_checksums(pkt);
  char *data = pkt->prepend(get_headers_size(*phv));
  int bytes_parsed = 0;
  for(auto it = headers.begin(); it != headers.end(); ++it) {
    Header &header = phv->get_header(*it);
    if(header.is_valid()){
      ELOGGER->deparser_emit(*pkt, *it);
      header.deparse(data + bytes_parsed);
      bytes_parsed += header.get_nbytes_packet();
      header.mark_invalid();
    }
  }
  ELOGGER->deparser_done(*pkt, *this);
}

void Deparser::update_checksums(Packet *pkt) const {
  PHV *phv = pkt->get_phv();
  for(const Checksum *checksum : checksums) {
    checksum->update(phv);
    ELOGGER->checksum_update(*pkt, *checksum);
  } 
}
