struct h<t> {
    t f;
};
bool g<t>(in t a) {
    h<t> v;
    v.f = a;
    return v.f == a;
}
bool gg()
{
    h<bit> a = { 0 };
    return g(a);
}

control c(out bool x) {
    apply {
        x = gg();
    }
}

control C(out bool b);
package top(C _c);

top(c()) main;
