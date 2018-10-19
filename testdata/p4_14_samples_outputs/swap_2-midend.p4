#include <core.p4>
#include <v1model.p4>

header hdr2_t {
    bit<8>  f1;
    bit<8>  f2;
    bit<16> f3;
}

struct metadata {
}

struct headers {
    @name(".hdr2") 
    hdr2_t hdr2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdr2_t>(hdr.hdr2);
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
    @name(".a21") action a21() {
        standard_metadata.egress_spec = 9w3;
    }
    @name(".a22") action a22() {
        standard_metadata.egress_spec = 9w4;
    }
    @name(".t_ingress_2") table t_ingress {
        actions = {
            a21();
            a22();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdr2.f1: exact @name("hdr2.f1") ;
        }
        size = 64;
        default_action = NoAction_0();
    }
    apply {
        t_ingress.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr2_t>(hdr.hdr2);
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

