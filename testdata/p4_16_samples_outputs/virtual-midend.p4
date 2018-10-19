extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    bit<16> tmp;
    @name("c.cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
    };
    @hidden action act() {
        tmp = cntr_0.f(16w6);
        p = tmp;
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

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

