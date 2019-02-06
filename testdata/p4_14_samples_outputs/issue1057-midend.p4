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
    bit<16> _meta_val160;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".ethernet") state ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".ethernet") direct_counter(CounterType.packets) ethernet_2;
    @name(".ethernet") action ethernet_3() {
        ethernet_2.count();
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name(".ethernet") table ethernet_5 {
        actions = {
            ethernet_3();
            @defaultonly NoAction_0();
        }
        counters = ethernet_2;
        default_action = NoAction_0();
    }
    apply {
        ethernet_5.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
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

