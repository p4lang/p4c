#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

struct metadata {
}

struct headers {
    @name("hdr1") 
    hdr1_t hdr1;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.hdr1);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_0 = false;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("a11") action a11() {
        bool hasReturned_2 = false;
        standard_metadata.egress_spec = 9w1;
    }
    @name("a12") action a12() {
        bool hasReturned_3 = false;
        standard_metadata.egress_spec = 9w2;
    }
    @name("t_ingress_1") table t_ingress_1() {
        actions = {
            a11;
            a12;
            NoAction;
        }
        key = {
            hdr.hdr1.f1: exact;
        }
        size = 128;
        default_action = NoAction();
    }

    apply {
        bool hasReturned_1 = false;
        t_ingress_1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_4 = false;
        packet.emit(hdr.hdr1);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_5 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasReturned_6 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
