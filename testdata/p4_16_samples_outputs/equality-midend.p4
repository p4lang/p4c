header H {
    bit<32>    a;
    varbit<32> b;
}

struct S {
    bit<32> a;
    H       h;
}

control c(out bit<1> x) {
    varbit<32> a_1;
    varbit<32> b_1;
    H h1;
    H h2;
    bit<32> s1_a;
    H s1_h;
    bit<32> s2_a;
    H s2_h;
    H[2] a1;
    H[2] a2;
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
        if (a_1 == b_1) 
            tbl_act.apply();
        else 
            if (!h1.isValid() && !h2.isValid() || h1.isValid() && h2.isValid() && h1.a == h2.a && h1.b == h2.b) 
                tbl_act_0.apply();
            else 
                if (s1_a == s2_a && (!s1_h.isValid() && !s2_h.isValid() || s1_h.isValid() && s2_h.isValid() && s1_h.a == s2_h.a && s1_h.b == s2_h.b)) 
                    tbl_act_1.apply();
                else 
                    if ((!a1[0].isValid() && !a2[0].isValid() || a1[0].isValid() && a2[0].isValid() && a1[0].a == a2[0].a && a1[0].b == a2[0].b) && (!a1[1].isValid() && !a2[1].isValid() || a1[1].isValid() && a2[1].isValid() && a1[1].a == a2[1].a && a1[1].b == a2[1].b)) 
                        tbl_act_2.apply();
                    else 
                        tbl_act_3.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

