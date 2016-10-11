#include <core.p4>

control c(inout bit<32> arg) {
    bit<32> tmp;
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
        if (t_0.apply(arg).hit) 
            arg = 32w0;
        else {
            tmp = arg + 32w1;
            arg = tmp;
        }
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
