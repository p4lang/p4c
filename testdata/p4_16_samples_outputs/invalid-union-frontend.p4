#include <core.p4>

header E {
}

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
    @name("c.h") H h_0;
    @name("c.a") action a() {
    }
    apply {
        a();
        if (hu.isValid()) {
            ;
        } else {
            h_0.setValid();
            h_0.f = 32w0;
            hu.h = h_0;
        }
    }
}

parser P(inout HU h);
control C(inout HU h);
package top(P _p, C _c);
top(p(), c()) main;
