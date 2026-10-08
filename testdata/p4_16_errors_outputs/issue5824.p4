#include <core.p4>

header h_t {
    bit<8> f;
}

struct Headers {
    h_t[1 < 32 > 0] h;
}

parser Parser<H>(packet_in packet, out H hdr) {
    state start {
        transition accept;
    }
}

package SimplePipeline<H>(Parser<H> p);
SimplePipeline<Headers>(Parser<Headers>()) main;
