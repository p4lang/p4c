#include <core.p4>

#define ARP_OPER_REQUEST 1

control c() {
    bool b;
    bit x;

    action drop() {}

    action multicast(in bit<32> multicast_group_id) {}

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
            ( true, ARP_OPER_REQUEST, true ) : multicast(2);
        }
    }

    apply {
        forward.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;
