#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".route_eth") action route_eth(@name("egress_spec") bit<9> egress_spec_1, @name("src_addr") bit<48> src_addr_1) {
        standard_metadata.egress_spec = egress_spec_1;
        hdr.ethernet.src_addr = src_addr_1;
    }
    @name(".noop") action noop() {
    }
    @name(".routing") table routing_0 {
        actions = {
            route_eth();
            noop();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.dst_addr: lpm @name("ethernet.dst_addr");
        }
        default_action = NoAction_1();
    }
    apply {
        routing_0.apply();
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
