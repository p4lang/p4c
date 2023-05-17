control c(inout bit<32> x) {
    @name("c.tmp") bit<32> tmp_1;
    @name("c.tmp") bit<32> tmp_5;
    @name("c.tmp") bit<32> tmp_6;
    @hidden action issue15442l2() {
        tmp_1 = x + 32w1;
    }
    @hidden action issue15442l2_0() {
        tmp_1 = x;
    }
    @hidden action issue15442l2_1() {
        tmp_5 = x + 32w4294967295;
    }
    @hidden action issue15442l2_2() {
        tmp_5 = x;
    }
    @hidden action issue15442l2_3() {
        tmp_6 = tmp_5;
    }
    @hidden action issue15442l2_4() {
        tmp_6 = tmp_1;
    }
    @hidden action issue15442l7() {
        x = tmp_6;
    }
    @hidden table tbl_issue15442l2 {
        actions = {
            issue15442l2();
        }
        const default_action = issue15442l2();
    }
    @hidden table tbl_issue15442l2_0 {
        actions = {
            issue15442l2_0();
        }
        const default_action = issue15442l2_0();
    }
    @hidden table tbl_issue15442l2_1 {
        actions = {
            issue15442l2_1();
        }
        const default_action = issue15442l2_1();
    }
    @hidden table tbl_issue15442l2_2 {
        actions = {
            issue15442l2_2();
        }
        const default_action = issue15442l2_2();
    }
    @hidden table tbl_issue15442l2_3 {
        actions = {
            issue15442l2_3();
        }
        const default_action = issue15442l2_3();
    }
    @hidden table tbl_issue15442l2_4 {
        actions = {
            issue15442l2_4();
        }
        const default_action = issue15442l2_4();
    }
    @hidden table tbl_issue15442l7 {
        actions = {
            issue15442l7();
        }
        const default_action = issue15442l7();
    }
    apply {
        if (x > x + 32w1) {
            tbl_issue15442l2.apply();
        } else {
            tbl_issue15442l2_0.apply();
        }
        if (x > x + 32w4294967295) {
            tbl_issue15442l2_1.apply();
        } else {
            tbl_issue15442l2_2.apply();
        }
        if (tmp_1 > tmp_5) {
            tbl_issue15442l2_3.apply();
        } else {
            tbl_issue15442l2_4.apply();
        }
        tbl_issue15442l7.apply();
    }
}

control ctr(inout bit<32> x);
package top(ctr _c);
top(c()) main;
