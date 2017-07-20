#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1>  drop;
    bit<8>  egress_port;
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
    bit<64> f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header vag_t {
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
    bit<64> f4;
}

struct __metadataImpl {
    @name("ing_metadata") 
    ingress_metadata_t  ing_metadata;
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("vag") 
    vag_t      vag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<vag_t>(hdr.vag);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_5() {
    }
    @name("NoAction") action NoAction_6() {
    }
    @name("NoAction") action NoAction_7() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".nop") action nop_4() {
    }
    @name(".nop") action nop_5() {
    }
    @name(".nop") action nop_6() {
    }
    @name(".ing_drop") action ing_drop_0() {
        meta.ing_metadata.drop = 1w1;
    }
    @name(".set_egress_port") action set_egress_port_0(bit<8> egress_port) {
        meta.ing_metadata.egress_port = egress_port;
    }
    @name(".set_f1") action set_f1_0(bit<8> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".set_f2") action set_f2_0(bit<16> f2) {
        meta.ing_metadata.f2 = f2;
    }
    @name(".set_f3") action set_f3_0(bit<32> f3) {
        meta.ing_metadata.f3 = f3;
    }
    @name(".set_f4") action set_f4_0(bit<64> f4) {
        meta.ing_metadata.f4 = f4;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop_0();
            ing_drop_0();
            set_egress_port_0();
            set_f1_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.vag.f1: exact @name("hdr.vag.f1") ;
        }
        size = 1024;
        default_action = NoAction_0();
    }
    @name(".i_t2") table i_t2 {
        actions = {
            nop_4();
            set_f2_0();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.vag.f2: exact @name("hdr.vag.f2") ;
        }
        size = 1024;
        default_action = NoAction_5();
    }
    @name(".i_t3") table i_t3 {
        actions = {
            nop_5();
            set_f3_0();
            @defaultonly NoAction_6();
        }
        key = {
            hdr.vag.f3: exact @name("hdr.vag.f3") ;
        }
        size = 1024;
        default_action = NoAction_6();
    }
    @name(".i_t4") table i_t4 {
        actions = {
            nop_6();
            set_f4_0();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.vag.f4: exact @name("hdr.vag.f4") ;
        }
        size = 1024;
        default_action = NoAction_7();
    }
    apply {
        i_t1.apply();
        if (meta.ing_metadata.f1 == hdr.vag.f1) {
            i_t2.apply();
            if (meta.ing_metadata.f2 == hdr.vag.f2) 
                i_t3.apply();
        }
        else 
            i_t4.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
