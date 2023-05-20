#include <core.p4>
#include <v1model.p4>

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

// Headers
header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16> etherType;
}

struct metadata {
    // empty
}

struct headers {
    ethernet_t ethernet;
}

// Parser
parser TestParser(
  packet_in packet,
  out headers hdr,
  inout metadata meta,
  inout standard_metadata_t standard_metadata) {

  state start {
    transition state1; // start by parsing the ethernet header
  }

  state state1 {
    transition select(packet.lookahead<bit<10>>()) {
      10w1: state2;
      default: accept;
    }
  }

  state state2 {
    // extract a header so the cursor moves before lookahead
    packet.extract(hdr.ethernet);
    transition select(packet.lookahead<bit<16>>()) {
      16w0: reject;
      default: accept;
    }
  }
}

// The control structures are ignored for generating the tc parser productions.
control NullIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {  }
}
control NullChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}
control NullDeparser(packet_out packet, in headers hdr) {
    apply {  }
}
control NullVerification(inout headers hdr, inout metadata meta) {
    apply {  }
}
control NullEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {  }
}

// The full switch
V1Switch(
TestParser(),
NullVerification(),
NullIngress(),
NullEgress(),
NullChecksum(),
NullDeparser()
) main;
