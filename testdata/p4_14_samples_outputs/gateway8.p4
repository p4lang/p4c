#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".setb1") action setb1(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop() {
    }
    @name(".test1") table test1 {
        actions = {
            setb1;
            noop;
        }
        key = {
            hdr.data.f1: exact;
        }
    }
    @name(".test2") table test2 {
        actions = {
            setb1;
            noop;
        }
        key = {
            hdr.data.f2: exact;
        }
    }
    apply {
        if (hdr.data.b2 == 8w1 || hdr.data.b2 == 8w2 || hdr.data.b2 == 8w5 || hdr.data.b2 == 8w10 || hdr.data.b2 == 8w17 || hdr.data.b2 == 8w19) {
            test1.apply();
        } else {
            test2.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

