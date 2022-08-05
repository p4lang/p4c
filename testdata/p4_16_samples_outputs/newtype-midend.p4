#include <core.p4>

typedef bit<32> B32;
typedef bit<32> N32;
struct S {
    B32 b;
    N32 n;
}

header H {
    N32 field;
}

control c(out B32 x) {
    @name("c.k") N32 k_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.t") table t_0 {
        actions = {
            NoAction_1();
        }
        key = {
            k_0: exact @name("k") ;
        }
        const default_action = NoAction_1();
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

control e(out B32 x);
package top(e _e);
top(c()) main;

