#include <core.p4>
#include <v1model.p4>

struct test_t {
    bit<8> f;
}

@name("test_t") header test_t_0 {
    bit<8> f;
}

struct metadata {
    @name(".test") 
    test_t test;
}

struct headers {
    @name(".test") 
    test_t_0 test;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".simple") action simple() {
        hdr.test.f = 8w0;
    }
    @name(".simple_tbl") table simple_tbl {
        actions = {
            simple();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        simple_tbl.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
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

