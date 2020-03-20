control c(inout bit<8> v) {
    bool hasReturned;
    bit<8> val_0;
    bool hasReturned_0;
    @hidden action issue2175l3() {
        val_0 = 8w1;
        hasReturned_0 = true;
    }
    @hidden action act() {
        hasReturned = false;
        val_0 = v;
        hasReturned_0 = false;
    }
    @hidden action issue2175l6() {
        val_0 = 8w2;
    }
    @hidden action issue2175l14() {
        v = 8w1;
        hasReturned = true;
    }
    @hidden action act_0() {
        v = val_0;
    }
    @hidden action issue2175l17() {
        v = 8w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue2175l3 {
        actions = {
            issue2175l3();
        }
        const default_action = issue2175l3();
    }
    @hidden table tbl_issue2175l6 {
        actions = {
            issue2175l6();
        }
        const default_action = issue2175l6();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue2175l14 {
        actions = {
            issue2175l14();
        }
        const default_action = issue2175l14();
    }
    @hidden table tbl_issue2175l17 {
        actions = {
            issue2175l17();
        }
        const default_action = issue2175l17();
    }
    apply {
        tbl_act.apply();
        if (v == 8w0) {
            tbl_issue2175l3.apply();
        }
        if (!hasReturned_0) {
            tbl_issue2175l6.apply();
        }
        tbl_act_0.apply();
        if (val_0 == 8w0) {
            tbl_issue2175l14.apply();
        }
        if (!hasReturned) {
            tbl_issue2175l17.apply();
        }
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;

