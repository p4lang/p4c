#include <core.p4>

header E {}
header H {
    bit<32> f;
}
header_union HU {
    E e;
    H h;
}

parser p(inout HU hu) {
    state start {
        hu = (HU){#};
        transition accept;
    }
}

control c(inout HU hu) {
    action a() {
    }

    apply {
        H h = (H){#};
        a();
        if (!hu.isValid()) {
            h.setValid();
            h.f = 0;
            hu.h = h;
        }
    }
}

parser P(inout HU h);
control C(inout HU h);
package top(P _p, C _c);

top(p(), c()) main;
