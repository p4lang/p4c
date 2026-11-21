control t(inout bit<32> b) {
    @hidden action issue304l10() {
        b = b + 32w1;
        b = b + 32w1;
    }
    @hidden table tbl_issue304l10 {
        actions = {
            issue304l10();
        }
        const default_action = issue304l10();
    }
    apply {
        tbl_issue304l10.apply();
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
