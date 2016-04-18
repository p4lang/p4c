#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

struct h {
    bit<1> b;
}

struct metadata {
    @name("m") 
    h m;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        transition accept;
    }
}

control c(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("x") action x() {
        bool hasReturned_1 = false;
    }
    @name("t") table t() {
        actions = {
            x;
            NoAction;
        }
        default_action = NoAction();
    }

    apply {
        bool hasReturned_0 = false;
        if (meta.m.b == 1w1) 
            t.apply();
    }
}

control d(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    c() c_0;
    apply {
        bool hasReturned_2 = false;
        c_0.apply(hdr, meta, standard_metadata);
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    d() d_0;
    apply {
        bool hasReturned_3 = false;
        d_0.apply(hdr, meta, standard_metadata);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_4 = false;
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_5 = false;
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_6 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasReturned_7 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
