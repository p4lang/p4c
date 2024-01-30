control c(out bit<4> result) {
    @hidden action issue3219l13() {
        result = 4w1;
    }
    @hidden table tbl_issue3219l13 {
        actions = {
            issue3219l13();
        }
        const default_action = issue3219l13();
    }
    apply {
        tbl_issue3219l13.apply();
    }
}

control _c(out bit<4> r);
package top(_c _c);
top(c()) main;
