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
    @hidden action struct_init1l18() {
        h_0.setValid();
    }
    @hidden action struct_init1l15() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
        h_0.setValid();
        h_0.setValid();
        h_0.b = 32w2;
    }
    @hidden action struct_init1l13() {
        h_0.setValid();
    }
    @hidden table tbl_struct_init1l13 {
        actions = {
            struct_init1l13();
        }
        const default_action = struct_init1l13();
    }
    @hidden table tbl_struct_init1l15 {
        actions = {
            struct_init1l15();
        }
        const default_action = struct_init1l15();
    }
    @hidden table tbl_struct_init1l18 {
        actions = {
            struct_init1l18();
        }
        const default_action = struct_init1l18();
    }
    apply {
        tbl_struct_init1l13.apply();
        if (meta._foo__v0 == 9w192) {
            tbl_struct_init1l15.apply();
            if (!h_0.isValid() && false || h_0.isValid() && false) {
                tbl_struct_init1l18.apply();
            }
        }
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;

