#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
}

header vag_t {
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
}

struct metadata {
    @name(".ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct headers {
    @name(".vag") 
    vag_t vag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<vag_t>(hdr.vag);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".e_t1") table e_t1 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f1: exact @name("vag.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        e_t1.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_f1") action set_f1(bit<8> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop();
            set_f1();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f1: exact @name("vag.f1") ;
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        i_t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<vag_t>(hdr.vag);
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

