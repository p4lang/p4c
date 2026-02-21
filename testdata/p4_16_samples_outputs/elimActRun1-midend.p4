enum simple_table_action_run_t {
    simple_table_dummy_action,
    simple_table_NoAction
}

enum simple_table_0_action_run_t {
    simple_table_0_dummy_action,
    simple_table_0_NoAction
}

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
    simple_table_action_run_t simple_table_action_run;
    simple_table_0_action_run_t simple_table_0_action_run;
    @name("ingress.hasReturned") bool hasReturned;
    bit<48> key_0;
    bit<48> key_1;
    @corelib @noWarn("unused") @name(".NoAction") action NoAction_1() {
        simple_table_action_run = simple_table_action_run_t.simple_table_NoAction;
    }
    @corelib @noWarn("unused") @name(".NoAction") action NoAction_2() {
        simple_table_0_action_run = simple_table_0_action_run_t.simple_table_0_NoAction;
    }
    @name("ingress.dummy_action") action dummy_action() {
        simple_table_action_run = simple_table_action_run_t.simple_table_dummy_action;
    }
    @name("ingress.dummy_action") action dummy_action_1() {
        simple_table_0_action_run = simple_table_0_action_run_t.simple_table_0_dummy_action;
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            key_0: exact @name("key");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            key_1: exact @name("key");
        }
        actions = {
            dummy_action_1();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @hidden action elimActRun1l19() {
        hasReturned = false;
        key_0 = 48w1;
    }
    @hidden action elimActRun1l27() {
        key_1 = 48w1;
    }
    @hidden action elimActRun1l38() {
        h.eth_hdr.src_addr = 48w4;
        hasReturned = true;
    }
    @hidden table tbl_elimActRun1l19 {
        actions = {
            elimActRun1l19();
        }
        const default_action = elimActRun1l19();
    }
    @hidden table tbl_elimActRun1l27 {
        actions = {
            elimActRun1l27();
        }
        const default_action = elimActRun1l27();
    }
    @hidden table tbl_elimActRun1l38 {
        actions = {
            elimActRun1l38();
        }
        const default_action = elimActRun1l38();
    }
    apply {
        tbl_elimActRun1l19.apply();
        simple_table.apply();
        switch (simple_table_action_run) {
            simple_table_action_run_t.simple_table_dummy_action: {
                tbl_elimActRun1l27.apply();
                simple_table_0.apply();
                switch (simple_table_0_action_run) {
                    simple_table_0_action_run_t.simple_table_0_dummy_action: {
                        tbl_elimActRun1l38.apply();
                    }
                    default: {
                    }
                }
            }
            default: {
            }
        }
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<Headers>(ingress()) main;
