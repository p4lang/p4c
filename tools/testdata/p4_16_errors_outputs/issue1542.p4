#include <core.p4>

control c() {
    bool b;
    bit<1> x;
    action drop() {
    }
    action multicast(in bit<32> multicast_group_id) {
    }
    table forward {
        key = {
            b: exact;
            x: exact;
            b: exact;
        }
        actions = {
            drop;
            multicast(1);
        }
        const default_action = multicast(1);
        const entries = {
                        (true, 1, true) : multicast(2);

        }

    }
    apply {
        forward.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;

