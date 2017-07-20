#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_7() {
    }
    @name("NoAction") action NoAction_8() {
    }
    @name("NoAction") action NoAction_9() {
    }
    @name("NoAction") action NoAction_10() {
    }
    @name("NoAction") action NoAction_11() {
    }
    @name(".do_b") action do_b_0() {
    }
    @name(".do_d") action do_d_0() {
    }
    @name(".do_e") action do_e_0() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".nop") action nop_5() {
    }
    @name(".nop") action nop_6() {
    }
    @name(".nop") action nop_7() {
    }
    @name(".nop") action nop_8() {
    }
    @name(".A") table A {
        actions = {
            do_b_0();
            do_d_0();
            do_e_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        default_action = NoAction_0();
    }
    @name(".B") table B {
        actions = {
            nop_0();
            @defaultonly NoAction_7();
        }
        default_action = NoAction_7();
    }
    @name(".C") table C {
        actions = {
            nop_5();
            @defaultonly NoAction_8();
        }
        default_action = NoAction_8();
    }
    @name(".D") table D_1 {
        actions = {
            nop_6();
            @defaultonly NoAction_9();
        }
        default_action = NoAction_9();
    }
    @name(".E") table E {
        actions = {
            nop_7();
            @defaultonly NoAction_10();
        }
        default_action = NoAction_10();
    }
    @name(".F") table F {
        actions = {
            nop_8();
            @defaultonly NoAction_11();
        }
        default_action = NoAction_11();
    }
    apply {
        switch (A.apply().action_run) {
            do_b_0: {
                B.apply();
                C.apply();
            }
            do_d_0: {
                D_1.apply();
                C.apply();
            }
            do_e_0: {
                E.apply();
            }
        }

        F.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
