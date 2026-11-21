#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct headers {
}

struct metadata {
    bit<8> test;
}

typedef bit<8> test_t;
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("IngressImpl.act") action act(@name("a") test_t a) {
    }
    @name("IngressImpl.test_table") table test_table_0 {
        key = {
            meta.test: exact @name("meta.test");
        }
        actions = {
            act();
            @defaultonly NoAction_1();
        }
        const entries = {
                        8w1 : act(8w1);
        }
        default_action = NoAction_1();
    }
    apply {
        test_table_0.apply();
    }
}

control VerifyChecksumImpl(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control EgressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
