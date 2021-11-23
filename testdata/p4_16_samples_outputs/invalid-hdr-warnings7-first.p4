header H1 {
    bit<32> a;
}

header H2 {
    bit<32> a;
}

header_union U {
    H1 h1;
    H2 h2;
}

control ct(inout bit<32> param);
package top(ct _ct);
control c(inout bit<32> x) {
    U u;
    H1[2] hs;
    U[2] us;
    action initialize(out U u1, out H1[2] hs1, out U[2] us1) {
        u1.h1.a = 32w1;
        u1.h2.a = 32w1;
        hs1[0].a = 32w1;
        hs1[1].a = 32w1;
        us1[0].h1.a = 32w1;
        us1[0].h2.a = 32w1;
        u1.h1.setValid();
        u1.h2.setValid();
        hs1[0].setValid();
        hs1[1].setValid();
        us1[0].h1.setValid();
        us1[0].h2.setValid();
    }
    action inout_action1(inout U u1, inout H1[2] hs1, inout U[2] us1) {
        u1.h1.a = 32w1;
        u1.h2.a = 32w1;
        hs1[0].a = 32w1;
        hs1[1].a = 32w1;
        us1[0].h1.a = 32w1;
        us1[0].h2.a = 32w1;
        hs1[0].setInvalid();
        u1.h1.setValid();
        us1[0].h1.setValid();
    }
    action inout_action2(inout U u1, inout H1[2] hs1, inout U[2] us1) {
        bit<1> i = 1w1;
        us1[i].h1.setInvalid();
        us1[i].h2.setValid();
    }
    action xor(in U u1, in H1[2] hs1, in U[2] us1, out bit<32> result) {
        result = u1.h1.a ^ u1.h2.a ^ hs1[0].a ^ hs1[1].a ^ us1[0].h1.a ^ us1[0].h2.a ^ us1[1].h1.a ^ us1[1].h2.a;
    }
    apply @noWarn("uninitialized_use") {
        u.h1.setValid();
        hs[0].setValid();
        us[0].h1.setValid();
        initialize(u, hs, us);
        u.h1.a = 32w1;
        u.h2.a = 32w1;
        hs[0].a = 32w1;
        hs[1].a = 32w1;
        us[0].h1.a = 32w1;
        us[0].h2.a = 32w1;
        inout_action1(u, hs, us);
        u.h1.a = 32w1;
        u.h2.a = 32w1;
        hs[0].a = 32w1;
        hs[1].a = 32w1;
        us[0].h1.a = 32w1;
        us[0].h2.a = 32w1;
        inout_action2(u, hs, us);
        xor(u, hs, us, x);
    }
}

top(c()) main;

