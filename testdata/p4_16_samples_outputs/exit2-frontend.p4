control ctrl() {
    bit<32> x_0;
    bit<32> a_0;
    bit<32> b_0;
    bit<32> c_0;
    bool tmp;
    @name("e") action e_0() {
        exit;
        x_0 = 32w1;
    }
    apply {
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
        tmp = a_0 == 32w0;
        if (tmp) {
            b_0 = 32w2;
            e_0();
            c_0 = 32w3;
        }
        else {
            b_0 = 32w3;
            e_0();
            c_0 = 32w4;
        }
        c_0 = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
