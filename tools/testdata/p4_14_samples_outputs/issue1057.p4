#include <core.p4>
#include <v1model.p4>

struct meta_t {
    bit<16> val16;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".meta") 
    meta_t meta;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ethernet") state ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ethernet") direct_counter(CounterType.packets) ethernet_1;
    @name(".ethernet") action ethernet_2() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name(".ethernet") action ethernet_4() {
        ethernet_1.count();
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name(".ethernet") table ethernet_3 {
        actions = {
            ethernet_4;
        }
        counters = ethernet_1;
    }
    apply {
        ethernet_3.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
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

