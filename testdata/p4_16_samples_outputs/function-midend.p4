control c(out bit<16> b) {
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<16> retval;
    @hidden action function4() {
        hasReturned = true;
        retval = 16w12;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action function9() {
        b = retval;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_function4 {
        actions = {
            function4();
        }
        const default_action = function4();
    }
    @hidden table tbl_function9 {
        actions = {
            function9();
        }
        const default_action = function9();
    }
    apply {
        tbl_act.apply();
        if (hasReturned) {
            ;
        } else {
            tbl_function4.apply();
        }
        tbl_function9.apply();
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

