#include <algorithm>

#include "fields.h"

int Field::extract(const char *data, int hdr_offset) {
  value_sync = false;

  if(hdr_offset == 0 && nbits % 8 == 0) {
    std::copy(data, data + nbytes, bytes.begin());
    return nbits;
  }

  int field_offset = (nbytes << 3) - nbits;
  int i;
  
  int offset = hdr_offset - field_offset;
  if (offset == 0) {
    std::copy(data, data + nbytes, bytes.begin());
    bytes[0] &= (0xFF >> field_offset);
  }
  else if (offset > 0) { /* shift left */
    for (i = 0; i < nbytes - 1; i++) {
      bytes[i] = (data[i] << offset) | (data[i + 1] >> (8 - offset));
    }
    bytes[i] = data[i] << offset;
    if((hdr_offset + nbits) > (nbytes << 3)) {
      bytes[i] |= (data[i + 1] >> (8 - offset));
    }
  }
  else { /* shift right */
    offset = -offset;
    bytes[0] = data[0] >> offset;
    for (i = 1; i < nbytes; i++) {
      bytes[i] = (data[i - 1] << (8 - offset)) | (data[i] >> offset);
    }
  }

  return nbits;
}

int Field::deparse(char *data, int hdr_offset) const {
  if(hdr_offset == 0 && nbits % 8 == 0) {
    std::copy(bytes.begin(), bytes.end(), data);
    return nbits;
  }

  int field_offset = (nbytes << 3) - nbits;
  int hdr_bytes = (hdr_offset + nbits + 7) / 8;

  int i;
  
  // zero out bits we are going to write in data[0]
  data[0] &= (~(0xFF >> hdr_offset));

  int offset = field_offset - hdr_offset;
  if (offset == 0) {
    std::copy(bytes.begin() + 1, bytes.begin() + hdr_bytes, data + 1);
    data[0] |= bytes[0];
  }
  else if (offset > 0) { /* shift left */
    /* this assumes that the packet was memset to 0, TODO: improve */
    for (i = 0; i < hdr_bytes - 1; i++) {
      data[i] |= (bytes[i] << offset) | (bytes[i + 1] >> (8 - offset));
    }
    int tail_offset = (hdr_bytes << 3) - (hdr_offset + nbits);
    data[i] &= ((1 << tail_offset) - 1);
    data[i] |= bytes[i] << offset;
    if((field_offset + nbits) > (hdr_bytes << 3)) {
      data[i] |= (bytes[i + 1] >> (8 - offset));
    }
  }
  else { /* shift right */
    offset = -offset;
    data[0] |= (bytes[0] >> offset);
    for (i = 1; i < hdr_bytes - 1; i++) {
      data[i] = (bytes[i - 1] << (8 - offset)) | (bytes[i] >> offset);
    }
    int tail_offset = (hdr_bytes >> 3) - (hdr_offset + nbits);
    data[i] &= ((1 << tail_offset) - 1);
    data[i] |= (bytes[i - 1] << (8 - offset)) | (bytes[i] >> offset);
  }

  return nbits;
}
