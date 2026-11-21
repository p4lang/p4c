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
    @name("MyIngress.tmp") bit<1> tmp;
    @name("MyIngress.a0") action a0(@name("some_param") bit<3> some_param) {
        if (some_param == 3w7) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        hdr.h0.f0 = tmp;
    }
    @name("MyIngress.t0") table t0_0 {
        actions = {
            a0();
        }
        default_action = a0(3w2);
    }
    apply {
        t0_0.apply();
    }
}

SimpleArch<headers>(MyIngress()) main;
