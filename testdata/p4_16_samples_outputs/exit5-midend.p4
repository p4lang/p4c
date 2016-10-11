control ctrl() {
    bool hasExited;
    bit<32> a;
    bit<32> b;
    bit<32> c;
    @name("e") action e_0() {
        hasExited = true;
    }
    @name("f") action f_0() {
    }
    @name("t") table t() {
        actions = {
            e_0();
            f_0();
        }
        default_action = e_0();
    }
    action act() {
        b = 32w2;
    }
    action act_0() {
        c = 32w3;
    }
    action act_1() {
        b = 32w3;
    }
    action act_2() {
        c = 32w4;
    }
    action act_3() {
        hasExited = false;
        a = 32w0;
        b = 32w1;
        c = 32w2;
    }
    action act_4() {
        c = 32w5;
    }
    table tbl_act() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_2() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_3() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    apply {
        tbl_act.apply();
        switch (t.apply().action_run) {
            e_0: {
                if (!hasExited) {
                    tbl_act_0.apply();
                    t.apply();
                    if (!hasExited) 
                        tbl_act_1.apply();
                }
            }
            f_0: {
                if (!hasExited) {
                    tbl_act_2.apply();
                    t.apply();
                    if (!hasExited) 
                        tbl_act_3.apply();
                }
            }
        }

        if (!hasExited) 
            tbl_act_4.apply();
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
