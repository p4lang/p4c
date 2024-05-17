#include <core.p4>

extern void foo();
extern void bar();
extern bit<8> baz();
control c() {
    @name("c.tmp") bit<8> tmp;
    @name(".a") action a_0() {
    }
    @name(".b") action b_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.t") table t_0 {
        actions = {
            a_0();
            b_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            tmp: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
            switch_0_case_1();
        }
        const default_action = switch_0_case_1();
        const entries = {
                        const 8w1 : switch_0_case();
                        const 8w4 : switch_0_case_0();
        }
    }
    @hidden action issue4656_const_fold_generic_switch_label_expr14() {
        foo();
    }
    @hidden action issue4656_const_fold_generic_switch_label_expr15() {
        bar();
    }
    @hidden action act() {
        tmp = baz();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue4656_const_fold_generic_switch_label_expr14 {
        actions = {
            issue4656_const_fold_generic_switch_label_expr14();
        }
        const default_action = issue4656_const_fold_generic_switch_label_expr14();
    }
    @hidden table tbl_issue4656_const_fold_generic_switch_label_expr15 {
        actions = {
            issue4656_const_fold_generic_switch_label_expr15();
        }
        const default_action = issue4656_const_fold_generic_switch_label_expr15();
    }
    apply {
        tbl_act.apply();
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_issue4656_const_fold_generic_switch_label_expr14.apply();
            }
            switch_0_case_0: {
                tbl_issue4656_const_fold_generic_switch_label_expr15.apply();
            }
            switch_0_case_1: {
            }
        }
        t_0.apply();
    }
}

control C();
package top(C c);
top(c()) main;
