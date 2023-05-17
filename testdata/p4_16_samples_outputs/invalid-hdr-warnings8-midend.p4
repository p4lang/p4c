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
    H1 u_0_h1;
    H2 u_0_h2;
    @name("c.hs") H1[2] hs_0;
    @name("c.us") U[2] us_0;
    H1 u1_2_h1;
    H2 u1_2_h2;
    @name("c.hs1") H1[2] hs1_2;
    @name("c.us1") U[2] us1_2;
    H1 u1_3_h1;
    H2 u1_3_h2;
    @name("c.hs1") H1[2] hs1_3;
    @name("c.us1") U[2] us1_3;
    H1 u1_5_h1;
    H2 u1_5_h2;
    @name("c.hs1") H1[2] hs1_5;
    @name("c.us1") U[2] us1_5;
    H1 u1_6_h1;
    H2 u1_6_h2;
    @name("c.hs1") H1[2] hs1_6;
    @name("c.us1") U[2] us1_6;
    @name("c.inout_action2") action inout_action2() {
        u1_3_h1.setValid();
        u1_3_h1.a = 32w1;
        u1_3_h2.setInvalid();
        u1_3_h2.setValid();
        u1_3_h2.a = 32w1;
        u1_3_h1.setInvalid();
        hs1_3[0].a = 32w1;
        hs1_3[1].a = 32w1;
        us1_3[0].h1.setValid();
        us1_3[0].h1.a = 32w1;
        us1_3[0].h2.setInvalid();
        us1_3[0].h2.setValid();
        us1_3[0].h2.a = 32w1;
        us1_3[0].h1.setInvalid();
        u1_3_h1.setValid();
        u1_3_h2.setInvalid();
        u1_3_h2.setValid();
        u1_3_h1.setInvalid();
        hs1_3[0].setValid();
        hs1_3[1].setValid();
        us1_3[0].h1.setValid();
        us1_3[0].h2.setInvalid();
        us1_3[0].h2.setValid();
        us1_3[0].h1.setInvalid();
        if (u1_3_h1.isValid()) {
            u1_2_h1.setValid();
            u1_2_h1 = u1_3_h1;
            u1_2_h2.setInvalid();
        } else {
            u1_2_h1.setInvalid();
        }
        if (u1_3_h2.isValid()) {
            u1_2_h2.setValid();
            u1_2_h2 = u1_3_h2;
            u1_2_h1.setInvalid();
        } else {
            u1_2_h2.setInvalid();
        }
        hs1_2[0] = hs1_3[0];
        hs1_2[1] = hs1_3[1];
        if (us1_3[0].h1.isValid()) {
            us1_2[0].h1.setValid();
            us1_2[0].h1 = us1_3[0].h1;
            us1_2[0].h2.setInvalid();
        } else {
            us1_2[0].h1.setInvalid();
        }
        if (us1_3[0].h2.isValid()) {
            us1_2[0].h2.setValid();
            us1_2[0].h2 = us1_3[0].h2;
            us1_2[0].h1.setInvalid();
        } else {
            us1_2[0].h2.setInvalid();
        }
        if (us1_3[1].h1.isValid()) {
            us1_2[1].h1.setValid();
            us1_2[1].h1 = us1_3[1].h1;
            us1_2[1].h2.setInvalid();
        } else {
            us1_2[1].h1.setInvalid();
        }
        if (us1_3[1].h2.isValid()) {
            us1_2[1].h2.setValid();
            us1_2[1].h2 = us1_3[1].h2;
            us1_2[1].h1.setInvalid();
        } else {
            us1_2[1].h2.setInvalid();
        }
        u1_2_h1.setValid();
        u1_2_h1.a = 32w1;
        u1_2_h2.setInvalid();
        u1_2_h2.setValid();
        u1_2_h2.a = 32w1;
        u1_2_h1.setInvalid();
        hs1_2[0].a = 32w1;
        hs1_2[1].a = 32w1;
        us1_2[0].h1.setValid();
        us1_2[0].h1.a = 32w1;
        us1_2[0].h2.setInvalid();
        us1_2[0].h2.setValid();
        us1_2[0].h2.a = 32w1;
        us1_2[0].h1.setInvalid();
        hs1_2[0].setInvalid();
        u1_2_h1.setValid();
        u1_2_h2.setInvalid();
        us1_2[0].h1.setValid();
        us1_2[0].h2.setInvalid();
        if (u1_2_h1.isValid()) {
            u1_5_h1.setValid();
            u1_5_h1 = u1_2_h1;
            u1_5_h2.setInvalid();
        } else {
            u1_5_h1.setInvalid();
        }
        if (u1_2_h2.isValid()) {
            u1_5_h2.setValid();
            u1_5_h2 = u1_2_h2;
            u1_5_h1.setInvalid();
        } else {
            u1_5_h2.setInvalid();
        }
        hs1_5[0] = hs1_2[0];
        hs1_5[1] = hs1_2[1];
        if (us1_2[0].h1.isValid()) {
            us1_5[0].h1.setValid();
            us1_5[0].h1 = us1_2[0].h1;
            us1_5[0].h2.setInvalid();
        } else {
            us1_5[0].h1.setInvalid();
        }
        if (us1_2[0].h2.isValid()) {
            us1_5[0].h2.setValid();
            us1_5[0].h2 = us1_2[0].h2;
            us1_5[0].h1.setInvalid();
        } else {
            us1_5[0].h2.setInvalid();
        }
        if (us1_3[1].h1.isValid()) {
            us1_5[1].h1.setValid();
            us1_5[1].h1 = us1_3[1].h1;
            us1_5[1].h2.setInvalid();
        } else {
            us1_5[1].h1.setInvalid();
        }
        if (us1_3[1].h2.isValid()) {
            us1_5[1].h2.setValid();
            us1_5[1].h2 = us1_3[1].h2;
            us1_5[1].h1.setInvalid();
        } else {
            us1_5[1].h2.setInvalid();
        }
        u1_5_h1.setValid();
        u1_5_h1.a = 32w1;
        u1_5_h2.setInvalid();
        u1_5_h2.setValid();
        u1_5_h2.a = 32w1;
        u1_5_h1.setInvalid();
        hs1_5[0].a = 32w1;
        hs1_5[1].a = 32w1;
        us1_5[0].h1.setValid();
        us1_5[0].h1.a = 32w1;
        us1_5[0].h2.setInvalid();
        us1_5[0].h2.setValid();
        us1_5[0].h2.a = 32w1;
        us1_5[0].h1.setInvalid();
        us1_5[1w1].h1.setInvalid();
        us1_5[1w1].h2.setValid();
        us1_5[1w1].h1.setInvalid();
        if (u1_5_h1.isValid()) {
            u_0_h1.setValid();
            u_0_h1 = u1_5_h1;
            u_0_h2.setInvalid();
        } else {
            u_0_h1.setInvalid();
        }
        if (u1_5_h2.isValid()) {
            u_0_h2.setValid();
            u_0_h2 = u1_5_h2;
            u_0_h1.setInvalid();
        } else {
            u_0_h2.setInvalid();
        }
        hs_0[0] = hs1_5[0];
        hs_0[1] = hs1_5[1];
        if (us1_5[0].h1.isValid()) {
            us_0[0].h1.setValid();
            us_0[0].h1 = us1_5[0].h1;
            us_0[0].h2.setInvalid();
        } else {
            us_0[0].h1.setInvalid();
        }
        if (us1_5[0].h2.isValid()) {
            us_0[0].h2.setValid();
            us_0[0].h2 = us1_5[0].h2;
            us_0[0].h1.setInvalid();
        } else {
            us_0[0].h2.setInvalid();
        }
        if (us1_5[1].h1.isValid()) {
            us_0[1].h1.setValid();
            us_0[1].h1 = us1_5[1].h1;
            us_0[1].h2.setInvalid();
        } else {
            us_0[1].h1.setInvalid();
        }
        if (us1_5[1].h2.isValid()) {
            us_0[1].h2.setValid();
            us_0[1].h2 = us1_5[1].h2;
            us_0[1].h1.setInvalid();
        } else {
            us_0[1].h2.setInvalid();
        }
    }
    @name("c.xor") action xor() {
        if (u_0_h1.isValid()) {
            u1_6_h1.setValid();
            u1_6_h1 = u_0_h1;
            u1_6_h2.setInvalid();
        } else {
            u1_6_h1.setInvalid();
        }
        if (u_0_h2.isValid()) {
            u1_6_h2.setValid();
            u1_6_h2 = u_0_h2;
            u1_6_h1.setInvalid();
        } else {
            u1_6_h2.setInvalid();
        }
        hs1_6[0] = hs_0[0];
        hs1_6[1] = hs_0[1];
        if (us_0[0].h1.isValid()) {
            us1_6[0].h1.setValid();
            us1_6[0].h1 = us_0[0].h1;
            us1_6[0].h2.setInvalid();
        } else {
            us1_6[0].h1.setInvalid();
        }
        if (us_0[0].h2.isValid()) {
            us1_6[0].h2.setValid();
            us1_6[0].h2 = us_0[0].h2;
            us1_6[0].h1.setInvalid();
        } else {
            us1_6[0].h2.setInvalid();
        }
        if (us_0[1].h1.isValid()) {
            us1_6[1].h1.setValid();
            us1_6[1].h1 = us_0[1].h1;
            us1_6[1].h2.setInvalid();
        } else {
            us1_6[1].h1.setInvalid();
        }
        if (us_0[1].h2.isValid()) {
            us1_6[1].h2.setValid();
            us1_6[1].h2 = us_0[1].h2;
            us1_6[1].h1.setInvalid();
        } else {
            us1_6[1].h2.setInvalid();
        }
        x = u1_6_h1.a ^ u1_6_h2.a ^ hs1_6[0].a ^ hs1_6[1].a ^ us1_6[0].h1.a ^ us1_6[0].h2.a ^ us1_6[1].h1.a ^ us1_6[1].h2.a;
    }
    @hidden action invalidhdrwarnings8l13() {
        u_0_h1.setInvalid();
        u_0_h2.setInvalid();
        hs_0[0].setInvalid();
        hs_0[1].setInvalid();
        us_0[0].h1.setInvalid();
        us_0[0].h2.setInvalid();
        us_0[1].h1.setInvalid();
        us_0[1].h2.setInvalid();
        u_0_h1.setValid();
        u_0_h2.setInvalid();
        hs_0[0].setValid();
        us_0[0].h1.setValid();
        us_0[0].h2.setInvalid();
    }
    @hidden table tbl_invalidhdrwarnings8l13 {
        actions = {
            invalidhdrwarnings8l13();
        }
        const default_action = invalidhdrwarnings8l13();
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
        tbl_invalidhdrwarnings8l13.apply();
        tbl_inout_action2.apply();
        tbl_xor.apply();
    }
}

top(c()) main;
