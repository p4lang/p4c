bit<1> f(@optional bit<1> a) {
    return a;
}
bit<1> g() {
    return f();
}
control ct();
control c() {
    apply {
        g();
    }
}

package pt(ct c);
pt(c()) main;
