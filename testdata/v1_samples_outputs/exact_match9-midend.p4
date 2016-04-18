#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

struct metadata {
}

struct headers {
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("noop") action noop() {
        bool hasReturned_0 = false;
    }
    @name("setb1") action setb1(bit<8> val) {
        bool hasReturned_1 = false;
        hdr.data.b1 = val;
    }
    @name("setb2") action setb2(bit<8> val) {
        bool hasReturned_2 = false;
        hdr.data.b2 = val;
    }
    @name("setb3") action setb3(bit<8> val) {
        bool hasReturned_3 = false;
        hdr.data.b3 = val;
    }
    @name("setb4") action setb4(bit<8> val) {
        bool hasReturned_4 = false;
        hdr.data.b4 = val;
    }
    @name("test1") table test1() {
        actions = {
            noop;
            setb1;
            setb2;
            setb3;
            setb4;
            NoAction;
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction();
    }

    apply {
        bool hasExited = false;
        if (hdr.data.f2 != 32w0) 
            test1.apply();
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
        packet.emit(hdr.data);
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
