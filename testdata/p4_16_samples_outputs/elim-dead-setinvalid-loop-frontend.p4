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
package top<T>(P<T> p, C<T> c1, C<T> c2, D<T> d);
parser ParserImpl(packet_in pkt, out headers_t hdr) {
    @name("ParserImpl.l") H l_0;
    state start {
        l_0.setInvalid();
        transition head;
    }
    state head {
        pkt.extract<H>(hdr.h1);
        transition select(l_0.isValid()) {
            true: accept;
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
    apply {
        c_0.setInvalid();
        c_0.setValid();
        for (i_0 = 8w0; i_0 < hdr.h1.x; i_0 = i_0 + 8w1) {
            if (c_0.isValid()) {
                hdr.h2.x = hdr.h2.x + 8w1;
            }
            c_0.setInvalid();
        }
    }
}

control forin(inout headers_t hdr) {
    @name("forin.d") H d_0;
    @name("forin.j") bit<8> j_0;
    apply {
        d_0.setInvalid();
        d_0.setValid();
        d_0.setInvalid();
        for (@name("forin.j") bit<8> j_0 in hdr.h1.x .. 8w3) {
            d_0.setValid();
        }
        if (d_0.isValid()) {
            hdr.h2.x = 8w2;
        }
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<headers_t>(hdr);
    }
}

top<headers_t>(ParserImpl(), ingress(), forin(), deparser()) main;
