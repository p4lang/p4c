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
    H l;
    state start {
        transition head;
    }
    state head {
        pkt.extract(hdr.h1);
        transition select(l.isValid()) {
            true: accept;
            default: body;
        }
    }
    state body {
        l.setValid();
        transition select(hdr.h1.x) {
            0: clear;
            default: head;
        }
    }
    state clear {
        l.setInvalid();
        transition head;
    }
}

control ingress(inout headers_t hdr) {
    apply {
        H c;
        c.setValid();
        for (bit<8> i = 0; i < hdr.h1.x; i = i + 1) {
            if (c.isValid()) {
                hdr.h2.x = hdr.h2.x + 1;
            }
            c.setInvalid();
        }
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr);
    }
}

top(ParserImpl(), ingress(), deparser()) main;
