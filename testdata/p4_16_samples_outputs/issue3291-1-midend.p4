struct h<t> {
    t f;
}

struct h_0 {
    bit<1> f;
}

control c(out bool x) {
    @name("c.v") h<h_0> v_0;
    @hidden action issue32911l17() {
        x = v_0.f.f == 1w0;
    }
    @hidden table tbl_issue32911l17 {
        actions = {
            issue32911l17();
        }
        const default_action = issue32911l17();
    }
    apply {
        tbl_issue32911l17.apply();
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;

