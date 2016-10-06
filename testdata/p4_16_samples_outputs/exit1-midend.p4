control ctrl() {
    bool hasExited;
    @name("a") bit<32> a;
    @name("b") bit<32> b;
    @name("c") bit<32> c;
    @name("tmp") bool tmp_0;
    action act() {
        b = 32w2;
        hasExited = true;
    }
    action act_0() {
        b = 32w3;
        hasExited = true;
    }
    action act_1() {
        hasExited = false;
        a = 32w0;
        b = 32w1;
        c = 32w2;
        tmp_0 = a == 32w0;
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
        if (tmp_0) {
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
