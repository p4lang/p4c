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
parser ParserImpl(packet_in pkt, out headers_t hdr) {
    H dead;
    H live;
    U u;
    state start {
        pkt.extract(dead);
        pkt.extract(u.b);
        pkt.extract(u.a);
        hdr.h1.setValid();
        hdr.h1.x = dead.x;
        transition select(live.isValid()) {
            true: accept;
            false: parse_h2;
        }
    }
    state parse_h2 {
        pkt.extract(hdr.h2);
        hdr.h2.x = (u.b.isValid() ? 8w1 : 8w0);
        transition accept;
    }
}

control ingress(inout headers_t hdr) {
    apply {
        H copy;
        hdr.h3 = copy;
        hdr.h2.setInvalid();
        hdr.h2.setInvalid();
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr);
    }
}

top(ParserImpl(), ingress(), deparser()) main;
