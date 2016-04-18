#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<2>  x1;
    bit<3>  pad0;
    bit<2>  x2;
    bit<5>  pad1;
    bit<1>  x3;
    bit<2>  pad2;
    bit<32> skip;
    bit<1>  x4;
    bit<1>  x5;
    bit<6>  pad3;
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
    @name("setf4") action setf4(bit<32> val) {
        bool hasReturned_1 = false;
        hdr.data.f4 = val;
    }
    @name("noop") action noop() {
        bool hasReturned_2 = false;
    }
    @name("test1") table test1() {
        actions = {
            setf4;
            noop;
            NoAction;
        }
        key = {
            hdr.data.f1: exact;
        }
        default_action = NoAction();
    }

    @name("test2") table test2() {
        actions = {
            setf4;
            noop;
            NoAction;
        }
        key = {
            hdr.data.f2: exact;
        }
        default_action = NoAction();
    }

    apply {
        bool hasReturned_0 = false;
        if (hdr.data.x1 == 2w1 && hdr.data.x4 == 1w0) 
            test1.apply();
        else 
            test2.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasReturned_3 = false;
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasReturned_4 = false;
        packet.emit(hdr.data);
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
