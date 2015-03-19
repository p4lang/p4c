#include <cstdint>

#include "bm_sim/checksums.h"

namespace checksum {

static inline unsigned short csum16(char *buf, size_t len) {
  uint64_t sum = 0;
  uint64_t *b = (uint64_t *) buf;
  uint32_t t1, t2;
  uint16_t t3, t4;
  uint8_t *tail;
  /* Main loop - 8 bytes at a time */
  while (len >= sizeof(uint64_t)) {
      uint64_t s = *b++;
      sum += s;
      if (sum < s) sum++;
      len -= 8;
  }
  /* Handle tail less than 8-bytes long */
  tail = (uint8_t *) b;
  if (len & 4) {
      uint32_t s = *(uint32_t *)tail;
      sum += s;
      if (sum < s) sum++;
      tail += 4;
  }
  if (len & 2) {
      uint16_t s = *(uint16_t *) tail;
      sum += s;
      if (sum < s) sum++;
      tail += 2;
  }
  if (len & 1) {
      uint8_t s = *(uint8_t *) tail;
      sum += s;
      if (sum < s) sum++;
  }
  /* Fold down to 16 bits */
  t1 = sum;
  t2 = sum >> 32;
  t1 += t2;
  if (t1 < t2) t1++;
  t3 = t1;
  t4 = t1 >> 16;
  t3 += t4;
  if (t3 < t4) t3++;
  return ~t3;
}

#define IPV4_HDR_MAX_LEN 60
#define IPV4_CSUM_OFFSET 10 // byte offset

void update_ipv4_csum(header_id_t ipv4_hdr_id, int ipv4_csum_field, PHV *phv) {
  static thread_local char buffer[60];
  Header &ipv4_hdr = phv->get_header(ipv4_hdr_id);
  Field &ipv4_csum = ipv4_hdr[ipv4_csum_field];
  ipv4_hdr.deparse(buffer);
  buffer[IPV4_CSUM_OFFSET] = 0; buffer[IPV4_CSUM_OFFSET + 1] = 0;
  unsigned short csum = csum16(buffer, ipv4_hdr.get_nbytes_packet());
  // csum is in network byte order
  ipv4_csum.set_bytes((char *) &csum, 2);
}

#undef IPV4_HDR_MAX_LEN
#undef IPV4_CSUM_OFFSET

}
