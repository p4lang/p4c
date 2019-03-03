#include <core.p4>

struct standard_metadata_t {
    bit<16> instance_type;
}

control c(inout standard_metadata_t sm, in bit<16> eth) {
    @name("c.a") action a() {
        sm.instance_type = eth;
    }
    @name("c.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout standard_metadata_t sm, in bit<16> eth);
package top(proto p);
top(c()) main;

