#include <core.p4>
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
    bit<32> _m_m00;
    bit<32> _m_m11;
    bit<32> _m_m22;
    bit<32> _m_m33;
    bit<32> _m_m44;
    bit<16> _m_m55;
    bit<16> _m_m66;
    bit<16> _m_m77;
    bit<16> _m_m88;
    bit<16> _m_m99;
    bit<8>  _m_m1010;
    bit<8>  _m_m1111;
    bit<8>  _m_m1212;
    bit<8>  _m_m1313;
    bit<8>  _m_m1414;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".setmeta") action setmeta(bit<32> v0, bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<16> v5, bit<16> v6) {
        meta._m_m00 = v0;
        meta._m_m11 = v1;
        meta._m_m22 = v2;
        meta._m_m33 = v3;
        meta._m_m44 = v4;
        meta._m_m55 = v5;
        meta._m_m66 = v6;
    }
    @name(".test1") table test1_0 {
        actions = {
            setmeta();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
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

