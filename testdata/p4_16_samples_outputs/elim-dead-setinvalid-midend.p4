#include <core.p4>

header H {
    bit<8> x;
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
    @hidden action elimdeadsetinvalid57() {
        copy_0.setInvalid();
        hdr.h3 = copy_0;
        hdr.h2.setInvalid();
        hdr.h2.setInvalid();
    }
    @hidden table tbl_elimdeadsetinvalid57 {
        actions = {
            elimdeadsetinvalid57();
        }
        const default_action = elimdeadsetinvalid57();
    }
    apply {
        tbl_elimdeadsetinvalid57.apply();
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    @hidden action elimdeadsetinvalid66() {
        pkt.emit<H>(hdr.h1);
        pkt.emit<H>(hdr.h2);
        pkt.emit<H>(hdr.h3);
    }
    @hidden table tbl_elimdeadsetinvalid66 {
        actions = {
            elimdeadsetinvalid66();
        }
        const default_action = elimdeadsetinvalid66();
    }
    apply {
        tbl_elimdeadsetinvalid66.apply();
    }
}

top<headers_t>(ParserImpl(), ingress(), deparser()) main;
