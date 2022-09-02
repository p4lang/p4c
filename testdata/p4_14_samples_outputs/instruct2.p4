#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

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
    @name(".do_add") action do_add() {
        hdr.data.b3 = (bit<8>)hdr.data.b1 + (bit<8>)hdr.data.b2;
    }
    @name(".do_and") action do_and() {
        hdr.data.b2 = (bit<8>)hdr.data.b3 & (bit<8>)hdr.data.b4;
    }
    @name(".do_or") action do_or() {
        hdr.data.b4 = (bit<8>)hdr.data.b3 | (bit<8>)hdr.data.b1;
    }
    @name(".do_xor") action do_xor() {
        hdr.data.b1 = (bit<8>)hdr.data.b2 ^ (bit<8>)hdr.data.b3;
    }
    @name(".test1") table test1 {
        actions = {
            do_add;
            do_and;
            do_or;
            do_xor;
        }
        key = {
            hdr.data.f1: exact;
        }
    }
    apply {
        test1.apply();
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

