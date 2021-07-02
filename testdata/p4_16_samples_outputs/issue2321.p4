#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

bit<16> bump_val(inout bit<16> x) {
    x = x | 7;
    return x >> 1;
}
control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    action my_drop() {
        mark_to_drop(stdmeta);
    }
    action a1(inout bit<48> x) {
        hdr.ethernet.srcAddr = 5;
        x = x - 7;
    }
    action a2(in bit<16> y, in bit<16> z) {
        hdr.ethernet.srcAddr[15:0] = y + z;
    }
    table t1 {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            a1(hdr.ethernet.dstAddr);
            a2(bump_val(hdr.ethernet.etherType), bump_val(hdr.ethernet.etherType));
            my_drop;
            NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        t1.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

