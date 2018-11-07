#include <core.p4>

parser P<H>(packet_in pkt, out H hdr);
control C();
package S<H>(P<H> p, C c);
header data_h {
    bit<32> da;
}

struct headers_t {
    data_h data;
}

parser MyP1(packet_in pkt, out headers_t hdr) {
    headers_t hdr_0;
    state start {
        hdr_0.data.setInvalid();
        transition MyP2_start;
    }
    state MyP2_start {
        transition reject;
    }
}

control MyC1() {
    apply {
    }
}

S<headers_t>(MyP1(), MyC1()) main;

