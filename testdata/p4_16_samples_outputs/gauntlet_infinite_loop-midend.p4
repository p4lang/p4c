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

parser p(packet_in packet, out headers hdr) {
    H tmp_nop;
    padding tmp_p;
    @name("p.tmp_0") padding tmp_0;
    state start {
        tmp_nop.setInvalid();
        tmp_p.setInvalid();
        tmp_0.setInvalid();
        transition sub_parser_next;
    }
    state sub_parser_next {
        transition select(tmp_0.p) {
            8w0: sub_parser_parse_hdr;
            default: start_0;
        }
    }
    state sub_parser_parse_hdr {
        packet.extract<H>(tmp_nop);
        transition sub_parser_next;
    }
    state start_0 {
        hdr.nop = tmp_nop;
        hdr.p = tmp_p;
        hdr.p = tmp_0;
        transition accept;
    }
}

parser Parser(packet_in b, out headers hdr);
package top(Parser p);
top(p()) main;
