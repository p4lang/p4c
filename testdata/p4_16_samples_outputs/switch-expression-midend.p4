#include <core.p4>

control c(inout bit<32> b) {
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            b: exact;
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
    @hidden action switchexpression13() {
        b = 32w1;
    }
    @hidden action switchexpression14() {
        b = 32w2;
    }
    @hidden action switchexpression16() {
        b = 32w3;
    }
    @hidden table tbl_switchexpression13 {
        actions = {
            switchexpression13();
        }
        const default_action = switchexpression13();
    }
    @hidden table tbl_switchexpression14 {
        actions = {
            switchexpression14();
        }
        const default_action = switchexpression14();
    }
    @hidden table tbl_switchexpression16 {
        actions = {
            switchexpression16();
        }
        const default_action = switchexpression16();
    }
    apply {
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_switchexpression13.apply();
            }
            switch_0_case_0: {
                tbl_switchexpression14.apply();
            }
            switch_0_case_1: {
                tbl_switchexpression16.apply();
            }
        }
    }
}

control ct(inout bit<32> b);
package top(ct _c);
top(c()) main;
