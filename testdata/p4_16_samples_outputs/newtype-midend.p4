#include <core.p4>

struct S {
    bit<32> b;
    bit<32> n;
}

header H {
    bit<32> field;
}

control c(out bit<32> x) {
    @name("c.k") bit<32> k_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.t") table t_0 {
        actions = {
            NoAction_1();
        }
        key = {
            k_0: exact @name("k");
        }
        default_action = NoAction_1();
    }
    @hidden action newtype34() {
        k_0 = 32w0;
        x = 32w0;
    }
    @hidden action newtype43() {
        x = 32w3;
    }
    @hidden table tbl_newtype34 {
        actions = {
            newtype34();
        }
        const default_action = newtype34();
    }
    @hidden table tbl_newtype43 {
        actions = {
            newtype43();
        }
        const default_action = newtype43();
    }
    apply {
        tbl_newtype34.apply();
        t_0.apply();
        tbl_newtype43.apply();
    }
}

control e(out bit<32> x);
package top(e _e);
top(c()) main;
