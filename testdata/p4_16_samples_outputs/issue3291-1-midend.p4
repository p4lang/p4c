struct h<t> {
    t f;
}

struct h_bit1 {
    bit<1> f;
}

struct h_h_bit1 {
    h_bit1 f;
}

control c(out bool x) {
    @hidden action issue32911l17() {
        x = true;
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
