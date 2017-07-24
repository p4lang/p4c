#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1>    drop;
    bit<8>    egress_port;
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header vag_t {
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

struct __metadataImpl {
    @name("ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("vag") 
    vag_t      vag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".e_t1") table e_t1 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        default_action = NoAction();
    }
    apply {
        e_t1.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".set_f1") action set_f1(bit<1024> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".set_f2") action set_f2(bit<512> f2) {
        meta.ing_metadata.f2 = f2;
    }
    @name(".set_f3") action set_f3(bit<256> f3) {
        meta.ing_metadata.f3 = f3;
    }
    @name(".set_f4") action set_f4(bit<128> f4) {
        meta.ing_metadata.f4 = f4;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop();
            set_f1();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f1: exact @name("hdr.vag.f1") ;
        }
        default_action = NoAction();
    }
    @name(".i_t2") table i_t2 {
        actions = {
            nop();
            set_f2();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f2: exact @name("hdr.vag.f2") ;
        }
        default_action = NoAction();
    }
    @name(".i_t3") table i_t3 {
        actions = {
            nop();
            set_f3();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f3: exact @name("hdr.vag.f3") ;
        }
        default_action = NoAction();
    }
    @name(".i_t4") table i_t4 {
        actions = {
            nop();
            set_f4();
            @defaultonly NoAction();
        }
        key = {
            hdr.vag.f4: ternary @name("hdr.vag.f4") ;
        }
        default_action = NoAction();
    }
    apply {
        i_t1.apply();
        i_t2.apply();
        i_t3.apply();
        i_t4.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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
