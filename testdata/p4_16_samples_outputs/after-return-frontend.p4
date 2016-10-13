control ctrl() {
    bit<32> a_0;
    bool tmp;
    apply {
        a_0 = 32w0;
        tmp = a_0 == 32w0;
        if (tmp) 
            return;
        else 
            return;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
