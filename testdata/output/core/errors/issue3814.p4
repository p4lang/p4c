void h(in bit<1> t) {
}
control g(in bit<1> t)(@optional bit<1> tt) {
    apply {
        h(tt);
    }
}

control ct();
control c() {
    g() y;
    apply {
        y.apply(1);
    }
}

package pt(ct c);
pt(c()) main;
