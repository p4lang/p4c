header E {
}

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
    @name("c.x") bit<32> x_0;
    @name("c.a") action a() {
        h = (H){#};
    }
    apply {
        h = (H){#};
        e = (E){#};
        x_0 = h.f;
        a();
        if (e.isValid()) {
            h.setValid();
            h.f = x_0;
        }
    }
}

parser P(inout H h);
control C(inout H h, inout E e);
package top(P _p, C _c);
top(p(), c()) main;
