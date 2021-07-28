control p(out bit<1> y) {
    @name("p.x") bit<1> x_2;
    @name("p.b") action b() {
        y = x_2;
    }
    @hidden action inline33() {
        x_2 = 1w1;
    }
    @hidden table tbl_inline33 {
        actions = {
            inline33();
        }
        const default_action = inline33();
    }
    @hidden table tbl_b {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_inline33.apply();
        tbl_b.apply();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

