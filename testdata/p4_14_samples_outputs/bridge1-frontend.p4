#include <core.p4>
#include <v1model.p4>

struct metadata_t {
    bit<8> val;
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

struct metadata {
    @name(".meta") 
    metadata_t meta;
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".copyb1") action copyb1() {
        hdr.data.b1 = meta.meta.val;
    }
    @name(".output") table output_0 {
        actions = {
            copyb1();
        }
        default_action = copyb1();
    }
    apply {
        output_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".setb1") action setb1(bit<8> val, bit<9> port) {
        meta.meta.val = val;
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".test1") table test1_0 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name(".test2") table test2_0 {
        actions = {
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            meta.meta.val: exact @name("meta.val") ;
        }
        default_action = NoAction_3();
    }
    apply {
        test1_0.apply();
        test2_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
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

