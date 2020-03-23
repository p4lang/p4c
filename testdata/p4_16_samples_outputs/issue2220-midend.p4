#include <core.p4>

struct S {
    bit<8> val;
}

control c(out S s) {
    @hidden action issue2220l12() {
        s.val = 8w0;
    }
    @hidden table tbl_issue2220l12 {
        actions = {
            issue2220l12();
        }
        const default_action = issue2220l12();
    }
    apply {
        tbl_issue2220l12.apply();
    }
}

control simple<T>(out T t);
package top<T>(simple<T> e);
top<S>(c()) main;

