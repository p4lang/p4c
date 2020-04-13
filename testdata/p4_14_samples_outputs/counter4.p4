#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name(".cntDum") counter<bit<8>>(32w200, CounterType.packets) cntDum;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".act") action act(bit<9> port, bit<8> idx) {
        standard_metadata.egress_spec = port;
        cntDum.count((bit<8>)idx);
    }
    @name(".tab1") table tab1 {
        actions = {
            act;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 70000;
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

