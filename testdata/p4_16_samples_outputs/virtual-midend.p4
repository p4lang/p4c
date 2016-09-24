extern Virtual {
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    @name("tmp") bit<16> tmp_1;
    @name("tmp_0") bit<16> tmp_2;
    @name("cntr") Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            tmp_1 = ix + 16w1;
            return tmp_1;
        }
    };
    action act() {
        tmp_2 = cntr.f(16w6);
        p = tmp_2;
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
