struct PortId_t {
    bit<9> _v;
}

struct metadata_t {
    bit<9> _foo__v0;
}

control I(inout metadata_t meta) {
    @hidden action struct_init18() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
    }
    @hidden table tbl_struct_init18 {
        actions = {
            struct_init18();
        }
        const default_action = struct_init18();
    }
    apply {
        if (meta._foo__v0 == 9w192) {
            tbl_struct_init18.apply();
        }
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;
