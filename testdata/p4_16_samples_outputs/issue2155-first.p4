#include <core.p4>

header data {
    bit<8> da;
    bit<8> db;
}

struct headers {
    data hdr;
}

struct metadata {
    bit<32> foo;
}

parser ParserImpl(packet_in b, out headers p, inout metadata m) {
    state start {
        b.extract<data>(p.hdr);
        m.foo = 32w1;
        transition select(p.hdr.da) {
            8w0xaa: parse_b;
            default: reject;
        }
    }
    state parse_b {
        m.foo = 32w2;
        transition accept;
    }
}

parser P<H, M>(packet_in b, out H h, inout M m);
package top<H, M>(P<H, M> p);
top<headers, metadata>(ParserImpl()) main;

