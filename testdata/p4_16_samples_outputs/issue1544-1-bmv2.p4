#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

bit<16> sometimes_dec(in bit<16> x) {
    bit<16> tmp;
    if (x > 5) {
        tmp = x - 1;
    } else {
        tmp = x;
    }
    return tmp;
}
struct metadata {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

action my_drop(inout standard_metadata_t smeta) {
    mark_to_drop(smeta);
}
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action set_port(bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    table mac_da {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            set_port;
            my_drop(standard_metadata);
        }
        default_action = my_drop(standard_metadata);
    }
    apply {
        mac_da.apply();
        hdr.ethernet.srcAddr[15:0] = sometimes_dec(hdr.ethernet.srcAddr[15:0]);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

