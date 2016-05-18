control ctrl() {
    bool hasExited;
    bit<32> a_0;
    bit<32> b_0;
    bit<32> c_0;
    action act() {
        b_0 = 32w2;
        hasExited = true;
    }
    action act_0() {
        b_0 = 32w3;
        hasExited = true;
    }
    action act_1() {
        hasExited = false;
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
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
    apply {
        tbl_act.apply();
        if (a_0 == 32w0) {
            tbl_act_0.apply();
        }
        else {
            tbl_act_1.apply();
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
