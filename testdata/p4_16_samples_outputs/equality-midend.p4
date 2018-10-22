header H {
    bit<32>    a;
    varbit<32> b;
}

struct S {
    bit<32> a;
    H       h;
}

control c(out bit<1> x) {
    varbit<32> a_0;
    varbit<32> b_0;
    H h1_0;
    H h2_0;
    bit<32> s1_0_a;
    H s1_0_h;
    bit<32> s2_0_a;
    H s2_0_h;
    H[2] a1_0;
    H[2] a2_0;
    @hidden action act() {
        x = 1w1;
    }
    @hidden action act_0() {
        x = 1w1;
    }
    @hidden action act_1() {
        x = 1w1;
    }
    @hidden action act_2() {
        x = 1w1;
    }
    @hidden action act_3() {
        x = 1w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    apply {
        if (a_0 == b_0) 
            tbl_act.apply();
        else 
            if (!h1_0.isValid() && !h2_0.isValid() || h1_0.isValid() && h2_0.isValid() && h1_0.a == h2_0.a && h1_0.b == h2_0.b) 
                tbl_act_0.apply();
            else 
                if (s1_0_a == s2_0_a && (!s1_0_h.isValid() && !s2_0_h.isValid() || s1_0_h.isValid() && s2_0_h.isValid() && s1_0_h.a == s2_0_h.a && s1_0_h.b == s2_0_h.b)) 
                    tbl_act_1.apply();
                else 
                    if ((!a1_0[0].isValid() && !a2_0[0].isValid() || a1_0[0].isValid() && a2_0[0].isValid() && a1_0[0].a == a2_0[0].a && a1_0[0].b == a2_0[0].b) && (!a1_0[1].isValid() && !a2_0[1].isValid() || a1_0[1].isValid() && a2_0[1].isValid() && a1_0[1].a == a2_0[1].a && a1_0[1].b == a2_0[1].b)) 
                        tbl_act_2.apply();
                    else 
                        tbl_act_3.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

