bit<16> max(in bit<16> left, in bit<16> right) {
    bool hasReturned_0;
    bit<16> retval_0;
    hasReturned_0 = false;
    if (left > right) {
        hasReturned_0 = true;
        retval_0 = left;
    }
    if (!hasReturned_0) {
        hasReturned_0 = true;
        retval_0 = right;
    }
    return retval_0;
}
control c(out bit<16> b) {
    bit<16> tmp_0;
    @hidden action act() {
        tmp_0 = max(16w10, 16w12);
        b = tmp_0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control ctr(out bit<16> b);
package top(ctr _c);
top(c()) main;

