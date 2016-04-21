control ctrl() {
    @name("a") bit<32> a_0_0;
    @name("b") bit<32> b_0_0;
    @name("c") bit<32> c_0_0;
    @name("hasReturned") bool hasReturned_0;
    apply {
        bool hasExited = false;
        hasReturned_0 = false;
        a_0_0 = 32w0;
        b_0_0 = 32w1;
        c_0_0 = 32w2;
        if (a_0_0 == 32w0) {
            b_0_0 = 32w2;
            hasReturned_0 = true;
        }
        else {
            b_0_0 = 32w3;
            hasReturned_0 = true;
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
