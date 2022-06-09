#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @name("IngressImpl.value") bit<32> value_1;
    @name("IngressImpl.value") bit<32> value_3;
    @name("IngressImpl.hasReturned") bool hasReturned;
    @name("IngressImpl.retval") bit<32> retval;
    @name("IngressImpl.hasReturned") bool hasReturned_1;
    @name("IngressImpl.retval") bit<32> retval_1;
    @name("IngressImpl.update_value") action update_value() {
        hasReturned = false;
        hasReturned = true;
        retval = 32w1;
        value_3 = retval;
        value_1 = value_3;
    }
    apply {
        hasReturned_1 = false;
        hasReturned_1 = true;
        retval_1 = 32w1;
        update_value();
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

