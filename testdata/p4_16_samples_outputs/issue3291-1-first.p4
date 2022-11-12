struct h<t> {
    t f;
}

bool g<t>(in t a) {
    h<t> v;
    v.f = a;
    return v.f == a;
}
struct h_0 {
    bit<1> f;
}

struct h_1 {
    h_0 f;
}

bool g_0(in h_0 a) {
    h_1 v;
    v.f = a;
    return v.f == a;
}
bool gg() {
    h_0 a = (h_0){f = 1w0};
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
