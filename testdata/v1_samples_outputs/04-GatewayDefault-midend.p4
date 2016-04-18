#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

struct ingress_metadata_t {
    bit<8>  drop;
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

struct metadata {
    @name("ing_metadata") 
    ingress_metadata_t ing_metadata;
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
    @name("nop") action nop() {
        bool hasReturned_0 = false;
    }
    @name("e_t1") table e_t1() {
        actions = {
            nop;
            NoAction;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        default_action = NoAction();
    }

    apply {
        bool hasExited = false;
        e_t1.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("nop") action nop() {
        bool hasReturned_1 = false;
    }
    @name("ing_drop") action ing_drop() {
        bool hasReturned_2 = false;
        meta.ing_metadata.drop = 8w1;
    }
    @name("set_f1") action set_f1(bit<8> f1) {
        bool hasReturned_3 = false;
        meta.ing_metadata.f1 = f1;
    }
    @name("set_f2") action set_f2(bit<16> f2) {
        bool hasReturned_4 = false;
        meta.ing_metadata.f2 = f2;
    }
    @name("set_f3") action set_f3(bit<32> f3) {
        bool hasReturned_5 = false;
        meta.ing_metadata.f3 = f3;
    }
    @name("set_egress_port") action set_egress_port(bit<8> egress_port) {
        bool hasReturned_6 = false;
        meta.ing_metadata.egress_port = egress_port;
    }
    @name("set_f4") action set_f4(bit<64> f4) {
        bool hasReturned_7 = false;
        meta.ing_metadata.f4 = f4;
    }
    @name("i_t1") table i_t1() {
        actions = {
            nop;
            ing_drop;
            set_f1;
            set_f2;
            set_f3;
            set_egress_port;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    @name("i_t2") table i_t2() {
        actions = {
            nop;
            set_f2;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    @name("i_t3") table i_t3() {
        actions = {
            nop;
            set_f3;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    @name("i_t4") table i_t4() {
        actions = {
            nop;
            set_f4;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    apply {
        bool hasExited_0 = false;
        switch (i_t1.apply().action_run) {
            default: {
                i_t4.apply();
            }
            nop: {
                i_t2.apply();
            }
            set_egress_port: {
                i_t3.apply();
            }
        }

    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasExited_1 = false;
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasExited_2 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasExited_3 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
