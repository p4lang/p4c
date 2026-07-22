control c(out bit<4> result) {
    @hidden action issue3219l19() {
        result = 4w1;
    }
    @hidden table tbl_issue3219l19 {
        actions = {
            issue3219l19();
        }
        const default_action = issue3219l19();
    }
    apply {
        tbl_issue3219l19.apply();
    }
}

control _c(out bit<4> r);
package top(_c _c);
top(c()) main;
