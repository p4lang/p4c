#include <core.p4>

header H {
    bit<8> a;
}

header padding {
    bit<8> p;
}

struct headers {
    H nop;
    padding p;
}

parser sub_parser(packet_in b, out headers hdr) {
    bit<8> tracker;

    state start {
        tracker = 0;
        transition next;
    }

    state next {
        if (tracker == 1) {
            b.extract(hdr.p);
            hdr.p.p = 1;
        }
        transition select(hdr.p.p) {
            0: parse_hdr;
            default: accept;
        }
    }
    state parse_hdr {
        tracker = tracker + 1;
        b.extract(hdr.nop);
        transition next;
    }

}

parser p(packet_in packet, out headers hdr) {
    state start {
        sub_parser.apply(packet, hdr);
        transition accept;
    }

}

parser Parser(packet_in b, out headers hdr);
package top(Parser p);
top(p()) main;
