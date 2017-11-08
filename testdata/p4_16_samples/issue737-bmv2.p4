#include <v1model.p4>

//------------------------------------------
header fixedLenHeader_h {
   bit<64> field;
}

//------------------------------------------
struct Meta {
   bool metafield;
}

struct Parsed_packet {
   fixedLenHeader_h h;
}

//------------------------------------------
parser TopParser(packet_in b, out Parsed_packet p, inout Meta m,
    inout standard_metadata_t metadata) {

  state start {
    m.metafield = true;
    transition accept;
  }
}

control VeryChecksum(inout Parsed_packet hdr, inout Meta meta) { apply {} }

control IngressP(inout Parsed_packet hdr,
                      inout Meta m,
                      inout standard_metadata_t standard_metadata) {

  apply {
    if (m.metafield) {
      hdr.h.field = 64w3;
    }
    if (m.metafield == false) {
      hdr.h.field = 64w5;
    }
    if (!m.metafield) {
      hdr.h.field = 64w4;
    }
  }
}

control EgressP(inout Parsed_packet hdr,
                     inout Meta meta,
                     inout standard_metadata_t standard_metadata) { apply {} }

control ChecksumComputer(inout Parsed_packet hdr,
                              inout Meta meta) { apply {} }

control TopDeparser(packet_out b, in Parsed_packet hdr) { apply {} }

V1Switch(TopParser(), VeryChecksum(), IngressP(), EgressP(), ChecksumComputer(),
    TopDeparser()) main;