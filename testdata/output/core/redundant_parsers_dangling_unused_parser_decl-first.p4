#include <core.p4>

header h_t {
    bit<16> f;
}

struct headers {
    h_t h1;
    h_t h2;
}

parser RedundantParser(inout headers hdr) {
    state start {
        transition accept;
    }
}

parser IParser2(packet_in packet, inout headers hdr) {
    state start {
        packet.extract<h_t>(hdr.h2);
        transition select(hdr.h2.f) {
            default: accept;
        }
    }
}

parser IParser(packet_in packet, inout headers hdr) {
    RedundantParser() redundant_parser;
    @name("IParser2") IParser2() IParser2_inst;
    state start {
        packet.extract<h_t>(hdr.h1);
        transition select(hdr.h1.f) {
            16w1: s1;
            default: s2;
        }
    }
    state s1 {
        redundant_parser.apply(hdr);
        transition accept;
    }
    state s2 {
        IParser2_inst.apply(packet, hdr);
        transition accept;
    }
}

parser Parser(packet_in p, inout headers hdr);
package top(Parser p);
top(IParser()) main;
