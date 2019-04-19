control c(inout bit<16> y) {
    bit<32> x_0;
    @name("c.a") action a() {
        y = (bit<16>)x_0;
    }
    @hidden action act() {
        x_0 = 32w10;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    apply {
        tbl_act.apply();
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

