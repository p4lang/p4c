struct h<t> {
    t f;
}

struct h_bit1 {
    bit<1> f;
}

struct h_h_bit1 {
    h_bit1 f;
}

bool g<t>(in t a) {
    h<t> v;
    v.f = a;
    return v.f == a;
}
bool g_0(in h_bit1 a) {
    h_h_bit1 v;
    v.f = a;
    return v.f == a;
}
bool gg() {
    h_bit1 a = (h_bit1){f = 1w0};
    return g_0(a);
}
control c(out bool x) {
    apply {
        x = gg();
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
