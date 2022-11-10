header E {}
header H {
    bit<32> f;
}

parser p(inout H h) {
    state start {
        h = (H){#};
        transition accept;
    }
}

control c(inout H h, inout E e) {
    action a() {
        h = (H){#};
    }

    apply {
        h = (H){#};
        e = (E){#};
        bit<32> x = h.f;
        a();
        if (e.isValid()) {
            h.setValid();
            h.f = x;
        }
    }
}

parser P(inout H h);
control C(inout H h, inout E e);
package top(P _p, C _c);

top(p(), c()) main;
