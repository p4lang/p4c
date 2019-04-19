#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct user_metadata_t {
}

parser myParser(packet_in pk, out headers_t hdr, inout user_metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pk.extract(hdr.ethernet);
        transition accept;
    }
}

control MyDeparser(packet_out pk, in headers_t hdr) {
    apply {
        pk.emit(hdr.ethernet);
    }
}

control ingress(inout headers_t hdr, inout user_metadata_t meta, inout standard_metadata_t stdmeta) {
    action a1() {
        hdr.ethernet.dstAddr = 1;
    }
    action a2() {
        hdr.ethernet.dstAddr = 1;
    }
    action a3() {
        hdr.ethernet.dstAddr = 1;
    }
    table t1 {
        key = {
            hdr.ethernet.etherType: exact;
        }
        actions = {
            @tableonly a1;
            a2;
            @defaultonly a3;
        }
        const entries = {
                        3 : a3();

        }

        default_action = a1;
    }
    apply {
        t1.apply();
    }
}

control egress(inout headers_t hdr, inout user_metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control verify_checksum(inout headers_t hdr, inout user_metadata_t meta) {
    apply {
    }
}

control compute_checksum(inout headers_t hdr, inout user_metadata_t meta) {
    apply {
    }
}

V1Switch(myParser(), verify_checksum(), ingress(), egress(), compute_checksum(), MyDeparser()) main;

