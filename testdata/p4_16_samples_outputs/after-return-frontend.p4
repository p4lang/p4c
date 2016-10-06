control ctrl() {
    @name("a") bit<32> a_0;
    @name("b") bit<32> b_0;
    @name("c") bit<32> c_0;
    bool tmp;
    apply {
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
        tmp = a_0 == 32w0;
        if (tmp) {
            b_0 = 32w2;
            return;
            c_0 = 32w3;
        }
        else {
            b_0 = 32w3;
            return;
            c_0 = 32w4;
        }
        c_0 = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
