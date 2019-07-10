struct PortId_t {
    bit<9> _v;
}

header H {
    bit<32> b;
}

struct metadata_t {
    bit<9> _foo__v0;
}

control I(inout metadata_t meta) {
    H h_0;
    @hidden action act() {
        h_0.setValid();
    }
    @hidden action act_0() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
        h_0.setValid();
        h_0.b = 32w2;
    }
    @hidden action act_1() {
        h_0.setValid();
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        if (meta._foo__v0 == 9w192) {
            tbl_act_0.apply();
            if (!h_0.isValid() && false || h_0.isValid() && true && false) {
                tbl_act_1.apply();
            }
        }
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;

