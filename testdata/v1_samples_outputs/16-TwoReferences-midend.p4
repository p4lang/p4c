#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("do_b") action do_b_0() {
    }
    @name("do_d") action do_d_0() {
    }
    @name("do_e") action do_e_0() {
    }
    @name("nop") action nop_0() {
    }
    @name("A") table A_0() {
        actions = {
            do_b_0;
            do_d_0;
            do_e_0;
            NoAction_0;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction_0();
    }
    @name("B") table B_0() {
        actions = {
            nop_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("C") table C_0() {
        actions = {
            nop_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("D") table D_0() {
        actions = {
            nop_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("E") table E_0() {
        actions = {
            nop_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("F") table F_0() {
        actions = {
            nop_0;
            NoAction_0;
        }
        default_action = NoAction_0();
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
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
