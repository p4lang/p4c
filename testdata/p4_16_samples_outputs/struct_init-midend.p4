struct PortId_t {
    bit<9> _v;
}

struct metadata_t {
    bit<9> _foo__v0;
}

control I(inout metadata_t meta) {
    @hidden action act() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (meta._foo__v0 == 9w192) 
            tbl_act.apply();
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;

