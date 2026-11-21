#include <core.p4>

control c(in bit<1> t) {
    table tt {
        actions = {
        }
    }
    apply {
        if (tt.apply().action_run == tt.apply().action_run) {
        }
    }
}

control ct(in bit<1> t);
package pt(ct c);
pt(c()) main;
