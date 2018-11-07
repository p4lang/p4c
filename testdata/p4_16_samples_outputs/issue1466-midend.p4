header hdr {
    bit<1> g;
}

control A(inout hdr _hdr) {
    hdr _hdr_0;
    @hidden action act() {
        _hdr_0 = _hdr;
        _hdr_0.g = 1w1;
        _hdr = _hdr_0;
        _hdr_0.g = 1w1;
        _hdr = _hdr_0;
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

