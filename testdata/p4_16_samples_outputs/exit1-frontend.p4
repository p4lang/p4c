control ctrl() {
    bit<32> a;
    apply {
        a = 32w0;
        if (a == 32w0) 
            exit;
        else 
            exit;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

