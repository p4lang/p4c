#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> f3;
    bit<16> f4;
    bit<8>  f5;
    bit<8>  f6;
    bit<4>  f7;
    bit<4>  f8;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name(".data")
    data_t     data;
    @name(".ethernet")
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".data") state data {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition data;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name(".route_eth") action route_eth(@name("egress_spec") bit<9> egress_spec_1, @name("src_addr") bit<48> src_addr_1) {
        standard_metadata.egress_spec = egress_spec_1;
        hdr.ethernet.src_addr = src_addr_1;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_1() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".setf2") action setf2(@name("val") bit<32> val) {
        hdr.data.f2 = val;
    }
    @name(".setf1") action setf1(@name("val") bit<32> val_2) {
        hdr.data.f1 = val_2;
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
    @name(".test1") table test1_0 {
        actions = {
            setf2();
            noop_1();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.data.f1: exact @name("data.f1");
        }
        default_action = NoAction_2();
    }
    @name(".test2") table test2_0 {
        actions = {
            setf1();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f2: exact @name("data.f2");
        }
        default_action = NoAction_3();
    }
    apply {
        routing_0.apply();
        if (hdr.data.f5 != hdr.data.f6) {
            test1_0.apply();
        } else {
            test2_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<data_t>(hdr.data);
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
