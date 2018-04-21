#include <core.p4>

typedef bit<32> B32;
newtype bit<32> N32;
struct S {
    B32 b;
    N32 n;
}

header H {
    N32 field;
}

control c(out B32 x) {
    N32 k;
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.t") table t {
        actions = {
            NoAction_0();
        }
        key = {
            k: exact @name("k") ;
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        x = 32w2;
    }
    @hidden action act_0() {
        k = (N32)32w0;
        x = (B32)(N32)32w0;
    }
    @hidden action act_1() {
        x = 32w3;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        tbl_act.apply();
        if ((N32)32w0 == (N32)32w1) 
            tbl_act_0.apply();
        t.apply();
        if (32w0 == (B32)(N32)32w0) 
            tbl_act_1.apply();
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

