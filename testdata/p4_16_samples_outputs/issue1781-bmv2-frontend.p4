#include <core.p4>
#include <v1model.p4>

struct headers {
}

struct metadata {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<32> value_0;
    bit<32> tmp;
    bit<32> tmp_0;
    @name("IngressImpl.update_value") action update_value(out bit<32> value_2) {
        {
            bool hasReturned = false;
            bit<32> retval;
            hasReturned = true;
            retval = 32w1;
            tmp = retval;
        }
        value_2 = tmp;
    }
    apply {
        {
            bool hasReturned_1 = false;
            bit<32> retval_1;
            hasReturned_1 = true;
            retval_1 = 32w1;
            tmp_0 = retval_1;
        }
        update_value(value_0);
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

