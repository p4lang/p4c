#include <core.p4>

header H {
    bit<8> x;
}

struct headers_t {
    H h1;
    H h2;
}

parser P<T>(packet_in pkt, out T hdr);
control C<T>(inout T hdr);
control D<T>(packet_out pkt, in T hdr);
package top<T>(P<T> p, C<T> c, D<T> d);
parser ParserImpl(packet_in pkt, out headers_t hdr) {
    @name("ParserImpl.l") H l_0;
    state start {
        l_0.setInvalid();
        transition head;
    }
    state head {
        pkt.extract<H>(hdr.h1);
        transition select((bit<1>)l_0.isValid()) {
            1w1: accept;
            default: body;
        }
    }
    state body {
        l_0.setValid();
        transition select(hdr.h1.x) {
            8w0: clear;
            default: head;
        }
    }
    state clear {
        l_0.setInvalid();
        transition head;
    }
}

control ingress(inout headers_t hdr) {
    @name("ingress.c") H c_0;
    @name("ingress.i") bit<8> i_0;
    @hidden action elimdeadsetinvalidloop61() {
        hdr.h2.x = hdr.h2.x + 8w1;
    }
    @hidden action elimdeadsetinvalidloop64() {
        c_0.setInvalid();
    }
    @hidden action elimdeadsetinvalidloop57() {
        c_0.setValid();
    }
    @hidden table tbl_elimdeadsetinvalidloop57 {
        actions = {
            elimdeadsetinvalidloop57();
        }
        const default_action = elimdeadsetinvalidloop57();
    }
    @hidden table tbl_elimdeadsetinvalidloop61 {
        actions = {
            elimdeadsetinvalidloop61();
        }
        const default_action = elimdeadsetinvalidloop61();
    }
    @hidden table tbl_elimdeadsetinvalidloop64 {
        actions = {
            elimdeadsetinvalidloop64();
        }
        const default_action = elimdeadsetinvalidloop64();
    }
    apply {
        tbl_elimdeadsetinvalidloop57.apply();
        for (i_0 = 8w0; i_0 < hdr.h1.x; i_0 = i_0 + 8w1) {
            if (c_0.isValid()) {
                tbl_elimdeadsetinvalidloop61.apply();
            }
            tbl_elimdeadsetinvalidloop64.apply();
        }
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    @hidden action elimdeadsetinvalidloop71() {
        pkt.emit<H>(hdr.h1);
        pkt.emit<H>(hdr.h2);
    }
    @hidden table tbl_elimdeadsetinvalidloop71 {
        actions = {
            elimdeadsetinvalidloop71();
        }
        const default_action = elimdeadsetinvalidloop71();
    }
    apply {
        tbl_elimdeadsetinvalidloop71.apply();
    }
}

top<headers_t>(ParserImpl(), ingress(), deparser()) main;
