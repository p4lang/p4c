#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata_t {
    bit<32> m0;
    bit<32> m1;
    bit<32> m2;
    bit<32> m3;
    bit<32> m4;
    bit<16> m5;
    bit<16> m6;
    bit<16> m7;
    bit<16> m8;
    bit<16> m9;
    bit<8>  m10;
    bit<8>  m11;
    bit<8>  m12;
    bit<8>  m13;
    bit<8>  m14;
}

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
    @name(".m")
    metadata_t m;
}

struct headers {
    @name(".data")
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".setmeta") action setmeta(@name("v0") bit<8> v0, @name("v1") bit<32> v1, @name("v2") bit<32> v2, @name("v3") bit<32> v3, @name("v4") bit<32> v4, @name("v5") bit<16> v5, @name("v6") bit<16> v6) {
        meta.m.m10 = v0;
        meta.m.m1 = v1;
        meta.m.m2 = v2;
        meta.m.m3 = v3;
        meta.m.m4 = v4;
        meta.m.m5 = v5;
        meta.m.m6 = v6;
    }
    @name(".test1") table test1_0 {
        actions = {
            setmeta();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.data.f1: exact @name("data.f1");
        }
        size = 8192;
        default_action = NoAction_1();
    }
    apply {
        test1_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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
