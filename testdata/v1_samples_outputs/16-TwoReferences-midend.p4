#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

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
        bool hasReturned_0 = false;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("do_b") action do_b() {
        bool hasReturned_2 = false;
    }
    @name("do_d") action do_d() {
        bool hasReturned_3 = false;
    }
    @name("do_e") action do_e() {
        bool hasReturned_4 = false;
    }
    @name("nop") action nop() {
        bool hasReturned_5 = false;
    }
    @name("A") table A() {
        actions = {
            do_b;
            do_d;
            do_e;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    @name("B") table B() {
        actions = {
            nop;
            NoAction;
        }
        default_action = NoAction();
    }

    @name("C") table C() {
        actions = {
            nop;
            NoAction;
        }
        default_action = NoAction();
    }

    @name("D") table D() {
        actions = {
            nop;
            NoAction;
        }
        default_action = NoAction();
    }

    @name("E") table E() {
        actions = {
            nop;
            NoAction;
        }
        default_action = NoAction();
    }

    @name("F") table F() {
        actions = {
            nop;
            NoAction;
        }
        default_action = NoAction();
    }

    apply {
        bool hasReturned_1 = false;
        switch (A.apply().action_run) {
            do_b: {
                B.apply();
                C.apply();
            }
            do_d: {
                D.apply();
                C.apply();
            }
            do_e: {
                E.apply();
            }
        }

        F.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_6 = false;
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_7 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasReturned_8 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
