#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

@command_line("--loopsUnroll") header H1 {
    bit<8> a;
}

header H2 {
    bit<16> b;
}

struct A {
    H1 u_x;
    H2 u_y;
    H1 hus0_x;
    H2 hus0_y;
    H1 hus1_x;
    H2 hus1_y;
}

struct B {
    H1 u_x_0;
    H2 u_y_0;
    H1 hus0_0_x;
    H2 hus0_0_y;
    H1 hus1_0_x;
    H2 hus1_0_y;
}

parser ParserImpl(packet_in pkt, out A hdr, inout B meta, inout standard_metadata_t sm) {
    state start {
        pkt.extract<H1>(hdr.u_x);
        pkt.extract<H1>(hdr.hus0_x);
        pkt.extract<H2>(hdr.hus1_y);
        pkt.extract<H1>(meta.u_x_0);
        pkt.extract<H1>(meta.hus0_0_x);
        pkt.extract<H2>(meta.hus1_0_y);
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
        hdr.u_x.a = a_1 + meta.u_x_0.a;
        hdr.hus0_x.a = b_1 + meta.hus0_0_x.a;
        hdr.hus1_y.b = c + meta.hus1_0_y.b;
    }
    @name("IngressImpl.t") table t_0 {
        key = {
            hdr.u_x.a      : exact @name("hdr.u.x.a");
            hdr.hus0_x.a   : exact @name("hdr.hus[0].x.a");
            hdr.hus1_y.b   : exact @name("hdr.hus[1].y.b");
            meta.u_x_0.a   : exact @name("meta.u.x.a");
            meta.hus0_0_x.a: exact @name("meta.hus[0].x.a");
            meta.hus1_0_y.b: exact @name("meta.hus[1].y.b");
        }
        actions = {
            rewrite();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @hidden action headerunionfieldnamecollisionbmv2l63() {
        sm.egress_spec = 9w1;
    }
    @hidden table tbl_headerunionfieldnamecollisionbmv2l63 {
        actions = {
            headerunionfieldnamecollisionbmv2l63();
        }
        const default_action = headerunionfieldnamecollisionbmv2l63();
    }
    apply {
        t_0.apply();
        tbl_headerunionfieldnamecollisionbmv2l63.apply();
    }
}

control EgressImpl(inout A hdr, inout B meta, inout standard_metadata_t sm) {
    apply {
    }
}

control DeparserImpl(packet_out pkt, in A hdr) {
    apply {
        pkt.emit<H1>(hdr.u_x);
        pkt.emit<H2>(hdr.u_y);
        pkt.emit<H1>(hdr.hus0_x);
        pkt.emit<H2>(hdr.hus0_y);
        pkt.emit<H1>(hdr.hus1_x);
        pkt.emit<H2>(hdr.hus1_y);
    }
}

V1Switch<A, B>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
