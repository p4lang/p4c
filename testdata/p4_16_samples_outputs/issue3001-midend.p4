header H {
    bit<8> x;
}

control c() {
    @name("c.h") H h_0;
    @hidden action issue3001l22() {
        h_0.setInvalid();
    }
    @hidden table tbl_issue3001l22 {
        actions = {
            issue3001l22();
        }
        const default_action = issue3001l22();
    }
    apply {
        tbl_issue3001l22.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

