header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
bit<32> f(out bit<32> a) {
    a = 32w0;
    return a;
}
bit<32> g(in bit<32> a) {
    return a;
}
control c(out bit<32> x, out bit<32> y) {
    action simple_action() {
        y = g(x);
        f(x);
        g(x);
    }
    apply {
        hdr h = (hdr){a = 32w0};
        bool b = h.isValid();
        b = h.isValid();
        b = h.isValid();
        b = h.isValid();
        h.isValid();
        h.isValid();
        h.isValid();
        h.setValid();
        if (b) {
            x = h.a;
            x = h.a;
            f(h.a);
            f(h.a);
            g(h.a);
            g(h.a);
        } else {
            x = f(h.a);
            x = g(h.a);
            f(h.a);
        }
        simple_action();
    }
}

top(c()) main;
