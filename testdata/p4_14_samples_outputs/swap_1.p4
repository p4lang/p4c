#include <core.p4>
#include <v1model.p4>

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

struct metadata {
}

struct headers {
    @name(".hdr1") 
    hdr1_t hdr1;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.hdr1);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".a11") action a11() {
        standard_metadata.egress_spec = 9w1;
    }
    @name(".a12") action a12() {
        standard_metadata.egress_spec = 9w2;
    }
    @name(".t_ingress_1") table t_ingress_1 {
        actions = {
            a11;
            a12;
        }
        key = {
            hdr.hdr1.f1: exact;
        }
        size = 128;
    }
    apply {
        t_ingress_1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.hdr1);
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

