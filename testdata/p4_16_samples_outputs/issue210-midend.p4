#include <core.p4>

control Ing(out bit<32> a) {
    @name("Ing.b") bool b_0;
    @name("Ing.cond") action cond() {
        if (b_0) {
            a = 32w5;
        } else {
            a = 32w10;
        }
    }
    @hidden action issue210l30() {
        b_0 = true;
    }
    @hidden table tbl_issue210l30 {
        actions = {
            issue210l30();
        }
        const default_action = issue210l30();
    }
    @hidden table tbl_cond {
        actions = {
            cond();
        }
        const default_action = cond();
    }
    apply {
        tbl_issue210l30.apply();
        tbl_cond.apply();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;
