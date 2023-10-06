control t(inout bit<32> b) {
    @hidden action issue304l19() {
        b = b + 32w1;
        b = b + 32w1;
    }
    @hidden table tbl_issue304l19 {
        actions = {
            issue304l19();
        }
        const default_action = issue304l19();
    }
    apply {
        tbl_issue304l19.apply();
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
