#include <core.p4>

@command_line("--preferSwitch") header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name("ingress.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.dummy_action") action dummy_action_1() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            48w1: exact @name("key");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            48w1: exact @name("key");
        }
        actions = {
            dummy_action_1();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        hasReturned = false;
        switch (simple_table.apply().action_run) {
            dummy_action: {
                switch (simple_table_0.apply().action_run) {
                    dummy_action_1: {
                        h.eth_hdr.src_addr = 48w4;
                        hasReturned = true;
                    }
                    default: {
                    }
                }
            }
            default: {
            }
        }
        if (hasReturned) {
            ;
        } else {
            exit;
        }
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<Headers>(ingress()) main;
