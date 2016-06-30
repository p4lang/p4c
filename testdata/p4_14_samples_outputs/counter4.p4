#include "/home/mbudiu/git/p4c/p4include/core.p4"
#include "/home/mbudiu/git/p4c/p4include/v1model.p4"

header ethernet_t {
    bit<48> dstAddr;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("cntDum") counter(32w200, CounterType.packets) cntDum;
    @name("act") action act(bit<48> idx) {
        hdr.ethernet.dstAddr = idx;
        cntDum.count((bit<32>)idx);
    }
    @name("tab1") table tab1() {
        actions = {
            act;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 70000;
        default_action = NoAction();
    }
    apply {
        tab1.apply();
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

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
