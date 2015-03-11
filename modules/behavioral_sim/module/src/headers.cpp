#include "behavioral_sim/headers.h"

Header::Header(const string &name, p4object_id_t id,
	       const HeaderType &header_type)
  : NamedP4Object(name, id), header_type(header_type) {
  valid = false;
  nbytes_phv = 0;
  nbytes_packet = 0;
  // header_type_id = header_type.get_type_id();
  for(int i = 0; i < header_type.get_num_fields(); i++) {
    // use emplace_back instead?
    fields.push_back(Field(header_type.get_bit_width(i)));
    nbytes_phv += fields.back().get_nbytes();
    nbytes_packet += fields.back().get_nbits();
  }
  assert(nbytes_packet % 8 == 0);
  nbytes_packet /= 8;
}

void Header::extract(const char *data) {
  int hdr_offset = 0;
  for(std::vector<Field>::iterator it = fields.begin();
      it != fields.end();
      ++it) {
    hdr_offset += it->extract(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
  mark_valid();
  return;
}

void Header::deparse(char *data) const {
  int hdr_offset = 0;
  for(std::vector<Field>::const_iterator it = fields.begin();
      it != fields.end();
      ++it) {
    hdr_offset += it->deparse(data, hdr_offset);
    data += hdr_offset / 8;
    hdr_offset = hdr_offset % 8;
  }
  return;
}
