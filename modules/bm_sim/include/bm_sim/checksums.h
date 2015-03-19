#ifndef _BM_CHECKSUMS_H_
#define _BM_CHECKSUMS_H_

#include "bm_sim/phv.h"

namespace checksum {

void update_ipv4_csum(header_id_t ipv4_hdr_id, int ipv4_csum_field, PHV *phv);

}


#endif
