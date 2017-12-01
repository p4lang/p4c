#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
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

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

