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
    @hidden action newtype40() {
        k_0 = 32w0;
        x = 32w0;
    }
    @hidden action newtype49() {
        x = 32w3;
    }
    @hidden table tbl_newtype40 {
        actions = {
            newtype40();
        }
        const default_action = newtype40();
    }
    @hidden table tbl_newtype49 {
        actions = {
            newtype49();
        }
        const default_action = newtype49();
    }
    apply {
        tbl_newtype40.apply();
        t_0.apply();
        tbl_newtype49.apply();
    }
}

control e(out bit<32> x);
package top(e _e);
top(c()) main;
