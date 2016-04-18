#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

struct ingress_metadata_t {
    bit<1> drop;
    bit<8> egress_port;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<15> ethertype;
}

header vag_t {
    bit<33> f1;
    bit<7>  f2;
}

struct metadata {
    @name("ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("vag") 
    vag_t      vag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.vag);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("nop") action nop() {
        bool hasReturned_1 = false;
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
        bool hasReturned_0 = false;
        e_t1.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("nop") action nop() {
        bool hasReturned_3 = false;
    }
    @name("set_egress_port") action set_egress_port(bit<8> egress_port) {
        bool hasReturned_4 = false;
        meta.ing_metadata.egress_port = egress_port;
    }
    @name("i_t1") table i_t1() {
        actions = {
            nop;
            set_egress_port;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }

    apply {
        bool hasReturned_2 = false;
        i_t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_5 = false;
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vag);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_6 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasReturned_7 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
