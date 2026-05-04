header H {
    bit<8> x;
}

control c() {
    @name("c.h") H h_0;
    @hidden action issue3001l13() {
        h_0.setInvalid();
    }
    @hidden table tbl_issue3001l13 {
        actions = {
            issue3001l13();
        }
        const default_action = issue3001l13();
    }
    apply {
        tbl_issue3001l13.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
