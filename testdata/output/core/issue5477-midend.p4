#include <core.p4>

parser ParserDef<H, M>(packet_in packet, out H hdr, inout M meta);
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
    hdr0_t    h0;
    hdr0_t[3] hs;
}

parser Parser(packet_in packet, out headers hdr, inout metadata meta) {
    state start {
        packet.extract<hdr0_t>(hdr.h0);
        meta.m0 = meta.m0 + hdr.h0.f0;
        transition reject;
    }
}

ParserOnlyArch<headers, metadata>(Parser()) main;
