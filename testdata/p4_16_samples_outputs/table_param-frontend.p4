#include <core.p4>

control c(inout bit<32> arg) {
    @name("a") action a_0() {
    }
    @name("t") table t_0(inout bit<32> x) {
        key = {
            x: exact;
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    apply {
        t_0.apply(arg);
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
