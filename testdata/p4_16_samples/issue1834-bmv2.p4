/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

struct headers { }

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
    action act(test_t a) { }    // did not work before PR #1792 was merged
    // action act(bit<8> a) { } // worked before and after PR #1792 was merged

    table test_table {
        key = {
            meta.test: exact;
        }
        actions = {
            act;
        }
        const entries = {
            1: act(1);
        }
    }

    apply {
        test_table.apply();
    }
}


control VerifyChecksumImpl(inout headers hdr, inout metadata meta) {
    apply { }
}


control EgressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply { }
}


control ComputeChecksumImpl(inout headers hdr, inout metadata meta) {
    apply { }
}


control DeparserImpl(packet_out packet, in headers hdr) {
    apply { }
}


V1Switch(
    ParserImpl(),
    VerifyChecksumImpl(),
    IngressImpl(),
    EgressImpl(),
    ComputeChecksumImpl(),
    DeparserImpl()
) main;
