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
        packet.extract<hdr1_t>(hdr.hdr1);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".a11") action a11() {
        standard_metadata.egress_spec = 9w1;
    }
    @name(".a12") action a12() {
        standard_metadata.egress_spec = 9w2;
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            a11();
            a12();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdr1.f1: exact @name("hdr1.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    apply {
        t_ingress.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr1_t>(hdr.hdr1);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

