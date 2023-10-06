control ctrl(out bit<3> _x);
package top(ctrl c);
control c_0(out bit<3> x) {
    @hidden action issue754l1() {
        x = 3w1;
    }
    @hidden table tbl_issue754l1 {
        actions = {
            issue754l1();
        }
        const default_action = issue754l1();
    }
    apply {
        tbl_issue754l1.apply();
    }
}

top(c_0()) main;
