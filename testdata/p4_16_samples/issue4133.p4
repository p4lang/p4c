#include <core.p4>

control MAPipe<H>(inout H hdr);
package SimpleArch<H>(MAPipe<H> mapipe);

header h0_t {
    bit<1> f0;
}

struct headers {
    h0_t h0;
}

control MyIngress(inout headers hdr) {

    action a0(bit<3> some_param) {
        hdr.h0.f0 = (some_param == 7 ? 1w1 : 1w0);
    }

    table t0 {
        actions = { a0; }
        default_action = a0(2);
    }

    apply {
       t0.apply();
    }
}

SimpleArch(MyIngress()) main;
