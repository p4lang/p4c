#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct headers {
}

struct metadata {
    bool test;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    register<bit<1>>(1) testRegister;
    table debug_table {
        key = {
            meta.test: exact;
        }
        actions = {
            NoAction;
        }
    }
    apply {
        bit<1> registerData;
        testRegister.read(registerData, 0);
        meta.test = (bool)registerData;
        debug_table.apply();
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

V1Switch(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;

