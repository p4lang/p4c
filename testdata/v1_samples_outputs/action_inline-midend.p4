#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

struct ht {
    bit<1> b;
}

struct metadata {
    @name("md") 
    ht md;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("b") action b() {
        @name("y0_0") bit<1> y0_0_0;
        @name("y0_1") bit<1> y0_1_0;
        {
            y0_0_0 = meta.md.b;
            y0_0_0 = y0_0_0 + 1w1;
            meta.md.b = y0_0_0;
        }
        {
            y0_1_0 = meta.md.b;
            y0_1_0 = y0_1_0 + 1w1;
            meta.md.b = y0_1_0;
        }
    }
    @name("t") table t() {
        actions = {
            b;
            NoAction;
        }
        default_action = NoAction();
    }

    apply {
        bool hasExited = false;
        t.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasExited_0 = false;
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasExited_1 = false;
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasExited_2 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasExited_3 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
