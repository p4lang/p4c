struct PortId_t {
    bit<9> _v;
}

struct metadata_t {
    PortId_t foo;
}

control I(inout metadata_t meta) {
    @hidden action act() {
        meta.foo._v = meta.foo._v + 9w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (meta.foo._v == 9w192) 
            tbl_act.apply();
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;

