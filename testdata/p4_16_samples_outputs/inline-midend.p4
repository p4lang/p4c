control p(out bit<1> y) {
    @name("p.b") action b() {
        y = 1w1;
    }
    @hidden table tbl_b {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_b.apply();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;
