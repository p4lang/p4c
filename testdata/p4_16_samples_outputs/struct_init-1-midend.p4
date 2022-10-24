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
    @name("I.h") H h_0;
    @hidden action struct_init1l15() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
        h_0.setValid();
    }
    @hidden table tbl_struct_init1l15 {
        actions = {
            struct_init1l15();
        }
        const default_action = struct_init1l15();
    }
    apply {
        if (meta._foo__v0 == 9w192) {
            tbl_struct_init1l15.apply();
        }
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;
