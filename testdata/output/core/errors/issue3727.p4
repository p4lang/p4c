void f2(in bit<1> a) {
}
action NoAction() {
}
control c(in bit<1> a) {
    bit<1> tt;
    table t {
        actions = {
            f2(1);
        }
    }
    apply {
        tt = 1;
        t.apply();
    }
}

control ct(in bit<1> a);
package pt(ct c);
pt(c()) main;
