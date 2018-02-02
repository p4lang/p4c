control ctrl() {
    bit<32> a;
    apply {
        bool hasReturned_0 = false;
        a = 32w0;
        if (a == 32w0) 
            hasReturned_0 = true;
        else 
            hasReturned_0 = true;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

