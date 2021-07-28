control p() {
    @name("p.x_1") bit<1> x_3;
    @name("p.b") action b() {
    }
    @hidden action decl2l32() {
        x_3 = 1w0;
    }
    @hidden table tbl_decl2l32 {
        actions = {
            decl2l32();
        }
        const default_action = decl2l32();
    }
    @hidden table tbl_b {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_decl2l32.apply();
        tbl_b.apply();
    }
}

control simple();
package m(simple pipe);
.m(.p()) main;

