control ctrl() {
    @name("ctrl.a") bit<32> a_0;
    apply {
        a_0 = 32w0;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
