header hdr {
    bit<1> g;
}

control A(inout hdr _hdr) {
    @hidden action act() {
        _hdr.g = 1w1;
        _hdr.g = 1w1;
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

control proto(inout hdr _hdr);
package top(proto p);
top(A()) main;

