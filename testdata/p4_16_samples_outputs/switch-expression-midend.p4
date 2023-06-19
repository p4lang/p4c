#include <core.p4>

control c(inout bit<32> b) {
    bit<32> switch_0_key;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
            switch_0_case_1();
        }
        const default_action = switch_0_case_1();
        const entries = {
                        const 32w16 : switch_0_case();
                        const 32w32 : switch_0_case();
                        const 32w64 : switch_0_case_0();
                        const 32w92 : switch_0_case_1();
        }
    }
    @hidden action switchexpression7() {
        b = 32w1;
    }
    @hidden action switchexpression8() {
        b = 32w2;
    }
    @hidden action switchexpression10() {
        b = 32w3;
    }
    @hidden action switchexpression5() {
        switch_0_key = b;
    }
    @hidden table tbl_switchexpression5 {
        actions = {
            switchexpression5();
        }
        const default_action = switchexpression5();
    }
    @hidden table tbl_switchexpression7 {
        actions = {
            switchexpression7();
        }
        const default_action = switchexpression7();
    }
    @hidden table tbl_switchexpression8 {
        actions = {
            switchexpression8();
        }
        const default_action = switchexpression8();
    }
    @hidden table tbl_switchexpression10 {
        actions = {
            switchexpression10();
        }
        const default_action = switchexpression10();
    }
    apply {
        tbl_switchexpression5.apply();
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_switchexpression7.apply();
            }
            switch_0_case_0: {
                tbl_switchexpression8.apply();
            }
            switch_0_case_1: {
                tbl_switchexpression10.apply();
            }
        }
    }
}

control ct(inout bit<32> b);
package top(ct _c);
top(c()) main;
