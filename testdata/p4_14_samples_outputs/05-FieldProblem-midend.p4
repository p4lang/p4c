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

struct __metadataImpl {
    @name("ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct __headersImpl {
    @name("vag") 
    vag_t vag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<vag_t>(hdr.vag);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".e_t1") table e_t1 {
        actions = {
            nop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.vag.f1: exact @name("hdr.vag.f1") ;
        }
        default_action = NoAction_0();
    }
    apply {
        e_t1.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_1() {
    }
    @name(".nop") action nop_1() {
    }
    @name(".set_f1") action set_f1_0(bit<8> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop_1();
            set_f1_0();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.vag.f1: exact @name("hdr.vag.f1") ;
        }
        size = 1024;
        default_action = NoAction_1();
    }
    apply {
        i_t1.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<vag_t>(hdr.vag);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
