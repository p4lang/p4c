#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

type bit<7> foo_t;
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control bar() {
    apply {
        const foo_t FOO = (foo_t)7w1;
        foo_t tmp = (foo_t)7w1;
    }
}

