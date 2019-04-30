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
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name(".c1") counter(32w1024, CounterType.packets) c1;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".count_c1_1") action count_c1_1() {
        c1.count((bit<32>)32w1);
    }
    @name(".count_c1_2") action count_c1_2() {
        c1.count((bit<32>)32w2);
    }
    @name(".t1") table t1 {
        actions = {
            count_c1_1;
        }
    }
    @name(".t2") table t2 {
        actions = {
            count_c1_2;
        }
    }
    apply {
        t1.apply();
        t2.apply();
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

