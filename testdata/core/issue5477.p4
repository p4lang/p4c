#include <core.p4>

parser ParserDef<H, M>(packet_in packet,
                out H hdr,
                inout M meta);
package ParserOnlyArch<H, M>(ParserDef<H, M> p);


header hdr0_t {
    bit<8> f0;
    bit<8> f1;
}

struct metadata {
    bit<8> m0;
    bit<8> m1;
}

struct headers {
    hdr0_t h0;
    hdr0_t[3] hs;
}

parser SubParserReject(packet_in packet,
                   inout metadata meta,
                   in bit<8> val) {
    state start {
        meta.m0 = meta.m0 + val;
        transition reject;
    }
}

parser Parser(packet_in packet,
                out headers hdr,
                inout metadata meta) {

    SubParserReject() subParser;

    state start {
        packet.extract(hdr.h0);
        subParser.apply(packet, meta, hdr.h0.f0);
        transition state0;
    }

    state state0 {
        packet.extract(hdr.hs.next);
        transition select(hdr.hs.last.f0) {
            0: state0;
            default: accept;
        }
    }
}

ParserOnlyArch(
    Parser()
) main;
