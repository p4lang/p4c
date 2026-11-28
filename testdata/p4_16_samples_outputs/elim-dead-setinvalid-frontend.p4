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
    @name("ParserImpl.u") U u_0;
    @name("ParserImpl.tmp") bit<8> tmp;
    state start {
        dead_0.setInvalid();
        live_0.setInvalid();
        u_0.a.setInvalid();
        u_0.b.setInvalid();
        pkt.extract<H>(dead_0);
        pkt.extract<H>(u_0.b);
        pkt.extract<H>(u_0.a);
        hdr.h1.setValid();
        hdr.h1.x = dead_0.x;
        transition select(live_0.isValid()) {
            true: accept;
            false: parse_h2;
        }
    }
    state parse_h2 {
        pkt.extract<H>(hdr.h2);
        transition select(u_0.b.isValid()) {
            true: parse_h2_true;
            false: parse_h2_false;
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
}

control ingress(inout headers_t hdr) {
    @name("ingress.copy") H copy_0;
    @name("ingress.us") U[2] us_0;
    @name("ingress.keep") H keep_0;
    @name("ingress.gone") H gone_0;
    apply {
        copy_0.setInvalid();
        hdr.h3 = copy_0;
        us_0[0].a.setInvalid();
        us_0[0].b.setInvalid();
        us_0[1].a.setInvalid();
        us_0[1].b.setInvalid();
        if (hdr.h1.x == 8w1) {
            us_0[0].a.setValid();
        }
        us_0[0].b.setInvalid();
        if (us_0[0].a.isValid()) {
            hdr.h1.x = 8w4;
        }
        keep_0.setInvalid();
        touch(keep_0);
        gone_0.setInvalid();
        touch(gone_0, 8w1);
        hdr.h2.setInvalid();
        hdr.h2.setInvalid();
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<headers_t>(hdr);
    }
}

top<headers_t>(ParserImpl(), ingress(), deparser()) main;
