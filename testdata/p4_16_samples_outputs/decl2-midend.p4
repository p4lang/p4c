control p() {
    @name("p.b") action b() {
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

control simple();
package m(simple pipe);
.m(.p()) main;
