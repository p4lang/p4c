#include <core.p4>

struct headers {
    bit<32> x;
}

extern void f(headers h);
control c() {
    @hidden action issue9331l19() {
        f((headers){x = 32w5});
    }
    @hidden table tbl_issue9331l19 {
        actions = {
            issue9331l19();
        }
        const default_action = issue9331l19();
    }
    apply {
        tbl_issue9331l19.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
