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
    data_h hdr_0_data;
    state start {
        hdr_0_data.setInvalid();
        transition reject;
    }
}

control MyC1() {
    apply {
    }
}

S<headers_t>(MyP1(), MyC1()) main;

