control c(inout bit<8> v) {
    @name("c.val_0") bit<8> val;
    @hidden action issue2175l9() {
        val = 8w1;
    }
    @hidden action issue2175l12() {
        val = 8w2;
    }
    @hidden action act() {
        val = v;
    }
    @hidden action issue2175l20() {
        v = 8w1;
    }
    @hidden action issue2175l23() {
        v = 8w2;
    }
    @hidden action act_0() {
        v = val;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue2175l9 {
        actions = {
            issue2175l9();
        }
        const default_action = issue2175l9();
    }
    @hidden table tbl_issue2175l12 {
        actions = {
            issue2175l12();
        }
        const default_action = issue2175l12();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue2175l20 {
        actions = {
            issue2175l20();
        }
        const default_action = issue2175l20();
    }
    @hidden table tbl_issue2175l23 {
        actions = {
            issue2175l23();
        }
        const default_action = issue2175l23();
    }
    apply {
        tbl_act.apply();
        if (v == 8w0) {
            tbl_issue2175l9.apply();
        } else {
            tbl_issue2175l12.apply();
        }
        tbl_act_0.apply();
        if (val == 8w0) {
            tbl_issue2175l20.apply();
        } else {
            tbl_issue2175l23.apply();
        }
    }
}

control e(inout bit<8> _v);
package top(e _e);
top(c()) main;
