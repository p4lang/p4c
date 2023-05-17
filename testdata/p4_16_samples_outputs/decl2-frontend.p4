control p() {
    @name("p.b") action b() {
    }
    apply {
        b();
    }
}

control simple();
package m(simple pipe);
.m(.p()) main;
