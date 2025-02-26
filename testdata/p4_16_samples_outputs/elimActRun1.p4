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
    action dummy_action() {
    }
    table simple_table_1 {
        key = {
            48w1: exact @name("key");
        }
        actions = {
            dummy_action();
        }
    }
    table simple_table_2 {
        key = {
            48w1: exact @name("key");
        }
        actions = {
            dummy_action();
        }
    }
    apply {
        switch (simple_table_1.apply().action_run) {
            dummy_action: {
                switch (simple_table_2.apply().action_run) {
                    dummy_action: {
                        h.eth_hdr.src_addr = 4;
                        return;
                    }
                }
            }
        }
        exit;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top(ingress()) main;
