control ctrl() {
    bit<32> a_0;
    apply {
        bool hasReturned = false;
        a_0 = 32w0;
        if (a_0 == 32w0) {
            hasReturned = true;
        } else {
            hasReturned = true;
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

