header h {
}

control c(out bool b) {
    @hidden action issue3779l16() {
        b = true;
    }
    @hidden table tbl_issue3779l16 {
        actions = {
            issue3779l16();
        }
        const default_action = issue3779l16();
    }
    apply {
        tbl_issue3779l16.apply();
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
