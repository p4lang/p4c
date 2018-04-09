#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<48> ingress_global_tstamp;
}

struct meta_t {
    bit<16> val16;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name(".meta") 
    meta_t               meta;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
        transition parse_ethernet;
    }
}

@name("test1_digest") struct test1_digest {
    bit<48>             dstAddr;
    standard_metadata_t standard_metadata;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".test_action") action test_action() {
        digest<test1_digest>((bit<32>)0x666, { hdr.ethernet.dstAddr, standard_metadata });
    }
    @name(".tbl0") table tbl0 {
        actions = {
            test_action;
        }
    }
    apply {
        tbl0.apply();
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

