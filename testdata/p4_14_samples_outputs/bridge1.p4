#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata_t {
    bit<8> val;
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

struct metadata {
    @name(".meta") 
    metadata_t meta;
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

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".copyb1") action copyb1() {
        hdr.data.b1 = (bit<8>)meta.meta.val;
    }
    @name(".output") table output {
        actions = {
            copyb1;
        }
        default_action = copyb1();
    }
    apply {
        output.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".setb1") action setb1(bit<8> val, bit<9> port) {
        meta.meta.val = val;
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
            noop;
        }
        key = {
            meta.meta.val: exact;
        }
    }
    apply {
        test1.apply();
        test2.apply();
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

