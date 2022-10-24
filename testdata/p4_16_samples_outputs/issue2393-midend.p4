#include <core.p4>

header ethernet_t {
    bit<48> src_addr;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name(".do_global_action") action do_global_action_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("h.eth_hdr.src_addr");
        }
        actions = {
            do_action();
            do_global_action_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        simple_table_0.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> _c);
top<Headers>(ingress()) main;
