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
    N32 k_0;
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.t") table t_0 {
        actions = {
            NoAction_0();
        }
        key = {
            k_0: exact @name("k") ;
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        k_0 = 32w0;
        x = 32w0;
    }
    @hidden action act_0() {
        x = 32w3;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        t_0.apply();
        tbl_act_0.apply();
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

