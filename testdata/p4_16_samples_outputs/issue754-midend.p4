control ctrl(out bit<3> _x);
package top(ctrl c);
control c_0(out bit<3> x) {
    @hidden action issue754l7() {
        x = 3w1;
    }
    @hidden table tbl_issue754l7 {
        actions = {
            issue754l7();
        }
        const default_action = issue754l7();
    }
    apply {
        tbl_issue754l7.apply();
    }
}

top(c_0()) main;
