#include <core.p4>

header H {
    bit<8> a;
}

header padding {
    bit<8> p;
}

struct headers {
    H       nop;
    padding p;
}

parser sub_parser(packet_in b, out headers hdr, out padding padding) {
    state start {
        transition next;
    }
    state next {
        transition select(padding.p) {
            8w0: parse_hdr;
            default: accept;
        }
    }
    state parse_hdr {
        b.extract<H>(hdr.nop);
        transition next;
    }
}

parser p(packet_in packet, out headers hdr) {
    @name("sub_parser") sub_parser() sub_parser_inst;
    state start {
        sub_parser_inst.apply(packet, hdr, hdr.p);
        transition accept;
    }
}

parser Parser(packet_in b, out headers hdr);
package top(Parser p);
top(p()) main;
