extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    bit<16> tmp_1;
    @name("cntr") Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
    };
    action act() {
        tmp_1 = cntr.f(16w6);
        p = tmp_1;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
