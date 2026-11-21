#include <core.p4>

control c() {
    @name("c.v") action v() {
    }
    @hidden table tbl_v {
        actions = {
            v();
        }
        const default_action = v();
    }
    apply {
        tbl_v.apply();
    }
}

control e();
package top(e e);
top(c()) main;
