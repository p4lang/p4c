#include <core.p4>

header H {
    bit<8> x;
}

header_union U {
    H a;
    H b;
}

struct headers_t {
    H h1;
    H h2;
    H h3;
}

parser P<T>(packet_in pkt, out T hdr);
control C<T>(inout T hdr);
control D<T>(packet_out pkt, in T hdr);
package top<T>(P<T> p, C<T> c, D<T> d);
extern void touch(inout H h);
extern void touch(out H h, in bit<8> v);
parser ParserImpl(packet_in pkt, out headers_t hdr) {
    @name("ParserImpl.dead") H dead_0;
    @name("ParserImpl.live") H live_0;
    H u_0_a;
    H u_0_b;
    @name("ParserImpl.tmp") bit<8> tmp;
    state start {
        live_0.setInvalid();
        pkt.extract<H>(dead_0);
        pkt.extract<H>(u_0_b);
        pkt.extract<H>(u_0_a);
        u_0_b.setInvalid();
        hdr.h1.setValid();
        hdr.h1.x = dead_0.x;
        transition select((bit<1>)live_0.isValid()) {
            1w1: accept;
            1w0: parse_h2;
            default: noMatch;
        }
    }
    state parse_h2 {
        pkt.extract<H>(hdr.h2);
        transition select((bit<1>)u_0_b.isValid()) {
            1w1: parse_h2_true;
            1w0: parse_h2_false;
            default: noMatch;
        }
    }
    state parse_h2_true {
        tmp = 8w1;
        transition parse_h2_join;
    }
    state parse_h2_false {
        tmp = 8w0;
        transition parse_h2_join;
    }
    state parse_h2_join {
        hdr.h2.x = tmp;
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control ingress(inout headers_t hdr) {
    @name("ingress.copy") H copy_0;
    @name("ingress.us") U[2] us_0;
    @name("ingress.keep") H keep_0;
    @name("ingress.gone") H gone_0;
    @hidden action elimdeadsetinvalid64() {
        us_0[0].a.setValid();
        us_0[0].b.setInvalid();
    }
    @hidden action elimdeadsetinvalid60() {
        copy_0.setInvalid();
        hdr.h3 = copy_0;
        us_0[0].a.setInvalid();
        us_0[0].b.setInvalid();
        us_0[1].a.setInvalid();
        us_0[1].b.setInvalid();
    }
    @hidden action elimdeadsetinvalid69() {
        hdr.h1.x = 8w4;
    }
    @hidden action elimdeadsetinvalid67() {
        us_0[0].b.setInvalid();
    }
    @hidden action elimdeadsetinvalid71() {
        keep_0.setInvalid();
        touch(keep_0);
        touch(gone_0, 8w1);
        hdr.h2.setInvalid();
        hdr.h2.setInvalid();
    }
    @hidden table tbl_elimdeadsetinvalid60 {
        actions = {
            elimdeadsetinvalid60();
        }
        const default_action = elimdeadsetinvalid60();
    }
    @hidden table tbl_elimdeadsetinvalid64 {
        actions = {
            elimdeadsetinvalid64();
        }
        const default_action = elimdeadsetinvalid64();
    }
    @hidden table tbl_elimdeadsetinvalid67 {
        actions = {
            elimdeadsetinvalid67();
        }
        const default_action = elimdeadsetinvalid67();
    }
    @hidden table tbl_elimdeadsetinvalid69 {
        actions = {
            elimdeadsetinvalid69();
        }
        const default_action = elimdeadsetinvalid69();
    }
    @hidden table tbl_elimdeadsetinvalid71 {
        actions = {
            elimdeadsetinvalid71();
        }
        const default_action = elimdeadsetinvalid71();
    }
    apply {
        tbl_elimdeadsetinvalid60.apply();
        if (hdr.h1.x == 8w1) {
            tbl_elimdeadsetinvalid64.apply();
        }
        tbl_elimdeadsetinvalid67.apply();
        if (us_0[0].a.isValid()) {
            tbl_elimdeadsetinvalid69.apply();
        }
        tbl_elimdeadsetinvalid71.apply();
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    @hidden action elimdeadsetinvalid82() {
        pkt.emit<H>(hdr.h1);
        pkt.emit<H>(hdr.h2);
        pkt.emit<H>(hdr.h3);
    }
    @hidden table tbl_elimdeadsetinvalid82 {
        actions = {
            elimdeadsetinvalid82();
        }
        const default_action = elimdeadsetinvalid82();
    }
    apply {
        tbl_elimdeadsetinvalid82.apply();
    }
}

top<headers_t>(ParserImpl(), ingress(), deparser()) main;
