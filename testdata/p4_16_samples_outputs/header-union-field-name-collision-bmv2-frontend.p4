#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

@command_line("--loopsUnroll") header H1 {
    bit<8> a;
}

header H2 {
    bit<16> b;
}

header_union HU {
    H1 x;
    H2 y;
}

struct A {
    HU    u;
    HU[2] hus;
}

struct B {
    HU    u;
    HU[2] hus;
}

parser ParserImpl(packet_in pkt, out A hdr, inout B meta, inout standard_metadata_t sm) {
    state start {
        pkt.extract<H1>(hdr.u.x);
        pkt.extract<H1>(hdr.hus[0].x);
        pkt.extract<H2>(hdr.hus[1].y);
        pkt.extract<H1>(meta.u.x);
        pkt.extract<H1>(meta.hus[0].x);
        pkt.extract<H2>(meta.hus[1].y);
        transition accept;
    }
}

control VerifyChecksumImpl(inout A hdr, inout B meta) {
    apply {
    }
}

control ComputeChecksumImpl(inout A hdr, inout B meta) {
    apply {
    }
}

control IngressImpl(inout A hdr, inout B meta, inout standard_metadata_t sm) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("IngressImpl.rewrite") action rewrite(@name("a") bit<8> a_1, @name("b") bit<8> b_1, @name("c") bit<16> c) {
        hdr.u.x.a = a_1 + meta.u.x.a;
        hdr.hus[0].x.a = b_1 + meta.hus[0].x.a;
        hdr.hus[1].y.b = c + meta.hus[1].y.b;
    }
    @name("IngressImpl.t") table t_0 {
        key = {
            hdr.u.x.a      : exact @name("hdr.u.x.a");
            hdr.hus[0].x.a : exact @name("hdr.hus[0].x.a");
            hdr.hus[1].y.b : exact @name("hdr.hus[1].y.b");
            meta.u.x.a     : exact @name("meta.u.x.a");
            meta.hus[0].x.a: exact @name("meta.hus[0].x.a");
            meta.hus[1].y.b: exact @name("meta.hus[1].y.b");
        }
        actions = {
            rewrite();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        t_0.apply();
        sm.egress_spec = 9w1;
    }
}

control EgressImpl(inout A hdr, inout B meta, inout standard_metadata_t sm) {
    apply {
    }
}

control DeparserImpl(packet_out pkt, in A hdr) {
    apply {
        pkt.emit<HU>(hdr.u);
        pkt.emit<HU[2]>(hdr.hus);
    }
}

V1Switch<A, B>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
