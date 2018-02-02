control c(inout bit<16> y) {
    bit<32> x;
    @name("c.a") action a_0() {
        y = (bit<16>)x;
    }
    @hidden action act() {
        x = 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_a {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_act.apply();
        tbl_a.apply();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

