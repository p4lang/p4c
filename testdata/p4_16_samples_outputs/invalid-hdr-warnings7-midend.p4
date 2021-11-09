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
    @name("c.u") U u_0;
    @name("c.hs") H1[2] hs_0;
    @name("c.us") U[2] us_0;
    @name("c.u1") U u1_0;
    @name("c.hs1") H1[2] hs1_0;
    @name("c.us1") U[2] us1_0;
    @name("c.u1") U u1_1;
    @name("c.hs1") H1[2] hs1_1;
    @name("c.us1") U[2] us1_1;
    @name("c.us1") U[2] us1_2;
    @name("c.u1") U u1_3;
    @name("c.hs1") H1[2] hs1_3;
    @name("c.us1") U[2] us1_3;
    @name("c.initialize") action initialize() {
        u1_0.h1.a = 32w1;
        u1_0.h2.a = 32w1;
        hs1_0[0].a = 32w1;
        hs1_0[1].a = 32w1;
        us1_0[0].h1.a = 32w1;
        us1_0[0].h2.a = 32w1;
        u1_0.h1.setValid();
        u1_0.h2.setValid();
        hs1_0[0].setValid();
        hs1_0[1].setValid();
        us1_0[0].h1.setValid();
        us1_0[0].h2.setValid();
        u_0.h1 = u1_0.h1;
        u_0.h2 = u1_0.h2;
        hs_0 = hs1_0;
        us_0 = us1_0;
    }
    @name("c.inout_action1") action inout_action1() {
        u1_1.h1 = u_0.h1;
        u1_1.h2 = u_0.h2;
        hs1_1 = hs_0;
        us1_1 = us_0;
        u1_1.h1.a = 32w1;
        u1_1.h2.a = 32w1;
        hs1_1[0].a = 32w1;
        hs1_1[1].a = 32w1;
        us1_1[0].h1.a = 32w1;
        us1_1[0].h2.a = 32w1;
        hs1_1[0].setInvalid();
        u1_1.h1.setValid();
        us1_1[0].h1.setValid();
        u_0.h1 = u1_1.h1;
        u_0.h2 = u1_1.h2;
        hs_0 = hs1_1;
        us_0 = us1_1;
    }
    @name("c.inout_action2") action inout_action2() {
        us1_2 = us_0;
        us1_2[1w1].h1.setInvalid();
        us1_2[1w1].h2.setValid();
        us_0 = us1_2;
    }
    @name("c.xor") action xor() {
        u1_3.h1 = u_0.h1;
        u1_3.h2 = u_0.h2;
        hs1_3 = hs_0;
        us1_3 = us_0;
        x = u1_3.h1.a ^ u1_3.h2.a ^ hs1_3[0].a ^ hs1_3[1].a ^ us1_3[0].h1.a ^ us1_3[0].h2.a ^ us1_3[1].h1.a ^ us1_3[1].h2.a;
    }
    @hidden action invalidhdrwarnings7l13() {
        u_0.h1.setInvalid();
        u_0.h2.setInvalid();
        hs_0[0].setInvalid();
        hs_0[1].setInvalid();
        us_0[0].h1.setInvalid();
        us_0[0].h2.setInvalid();
        us_0[1].h1.setInvalid();
        us_0[1].h2.setInvalid();
        u_0.h1.setValid();
        hs_0[0].setValid();
        us_0[0].h1.setValid();
    }
    @hidden action invalidhdrwarnings7l68() {
        u_0.h1.a = 32w1;
        u_0.h2.a = 32w1;
        hs_0[0].a = 32w1;
        hs_0[1].a = 32w1;
        us_0[0].h1.a = 32w1;
        us_0[0].h2.a = 32w1;
    }
    @hidden action invalidhdrwarnings7l78() {
        u_0.h1.a = 32w1;
        u_0.h2.a = 32w1;
        hs_0[0].a = 32w1;
        hs_0[1].a = 32w1;
        us_0[0].h1.a = 32w1;
        us_0[0].h2.a = 32w1;
    }
    @hidden table tbl_invalidhdrwarnings7l13 {
        actions = {
            invalidhdrwarnings7l13();
        }
        const default_action = invalidhdrwarnings7l13();
    }
    @hidden table tbl_initialize {
        actions = {
            initialize();
        }
        const default_action = initialize();
    }
    @hidden table tbl_invalidhdrwarnings7l68 {
        actions = {
            invalidhdrwarnings7l68();
        }
        const default_action = invalidhdrwarnings7l68();
    }
    @hidden table tbl_inout_action1 {
        actions = {
            inout_action1();
        }
        const default_action = inout_action1();
    }
    @hidden table tbl_invalidhdrwarnings7l78 {
        actions = {
            invalidhdrwarnings7l78();
        }
        const default_action = invalidhdrwarnings7l78();
    }
    @hidden table tbl_inout_action2 {
        actions = {
            inout_action2();
        }
        const default_action = inout_action2();
    }
    @hidden table tbl_xor {
        actions = {
            xor();
        }
        const default_action = xor();
    }
    apply @noWarn("uninitialized_use") {
        tbl_invalidhdrwarnings7l13.apply();
        tbl_initialize.apply();
        tbl_invalidhdrwarnings7l68.apply();
        tbl_inout_action1.apply();
        tbl_invalidhdrwarnings7l78.apply();
        tbl_inout_action2.apply();
        tbl_xor.apply();
    }
}

top(c()) main;

