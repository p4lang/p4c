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
    @name(".do_b") action do_b_0() {
    }
    @name(".do_d") action do_d_0() {
    }
    @name(".do_e") action do_e_0() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".A") table A_0 {
        actions = {
            do_b_0();
            do_d_0();
            do_e_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        default_action = NoAction();
    }
    @name(".B") table B_0 {
        actions = {
            nop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".C") table C_0 {
        actions = {
            nop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".D") table D_0 {
        actions = {
            nop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".E") table E_0 {
        actions = {
            nop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".F") table F_0 {
        actions = {
            nop_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        switch (A_0.apply().action_run) {
            do_b_0: {
                B_0.apply();
                C_0.apply();
            }
            do_d_0: {
                D_0.apply();
                C_0.apply();
            }
            do_e_0: {
                E_0.apply();
            }
        }

        F_0.apply();
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
