control ctrl() {
    @name("a") bit<32> a_0_0;
    @name("b") bit<32> b_0_0;
    @name("c") bit<32> c_0_0;
    @name("hasReturned") bool hasReturned_0;
    action act() {
        b_0_0 = 32w2;
        hasReturned_0 = true;
    }
    action act_0() {
        b_0_0 = 32w3;
        hasReturned_0 = true;
    }
    action act_1() {
        hasReturned_0 = false;
        a_0_0 = 32w0;
        b_0_0 = 32w1;
        c_0_0 = 32w2;
    }
    table tbl_act_1() {
        actions = {
            act_1;
        }
        const default_action = act_1();
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act_1.apply();
        if (a_0_0 == 32w0) {
            tbl_act.apply();
        }
        else {
            tbl_act_0.apply();
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
