#ifndef _BM_DEPARSER_H_
#define _BM_DEPARSER_H_

#include "phv.h"

using std::vector;

class Deparser {
private:
  vector<header_id_t> headers;

public:
  Deparser() {}

  void add_header(header_id_t header_id) {
    headers.push_back(header_id);
  }

  void deparse(const PHV &phv, char *data) const;

};

#endif
