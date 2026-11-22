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
    @name("p.sub_parser.tracker") bit<8> sub_parser_tracker;
    state start {
        hdr.nop.setInvalid();
        hdr.p.setInvalid();
        sub_parser_tracker = 8w0;
        transition sub_parser_next;
    }
    state sub_parser_next {
        transition select((bit<1>)(sub_parser_tracker == 8w1)) {
            1w1: sub_parser_next_true;
            1w0: sub_parser_next_join;
            default: noMatch;
        }
    }
    state sub_parser_next_true {
        packet.extract<padding>(hdr.p);
        hdr.p.p = 8w1;
        transition sub_parser_next_join;
    }
    state sub_parser_next_join {
        transition select(hdr.p.p) {
            8w0: sub_parser_parse_hdr;
            default: start_0;
        }
    }
    state sub_parser_parse_hdr {
        sub_parser_tracker = sub_parser_tracker + 8w1;
        packet.extract<H>(hdr.nop);
        transition sub_parser_next;
    }
    state start_0 {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser Parser(packet_in b, out headers hdr);
package top(Parser p);
top(p()) main;
