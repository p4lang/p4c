#ifndef _BM_DEPARSER_H_
#define _BM_DEPARSER_H_

#include "phv.h"
#include "packet.h"

using std::vector;

class Deparser {
private:
  vector<header_id_t> headers;

public:
  Deparser() {}

  void push_back_header(header_id_t header_id) {
    headers.push_back(header_id);
  }

  size_t get_headers_size(const PHV &phv) const;

  void deparse(const PHV &phv, Packet *pkt) const;

};

#endif
