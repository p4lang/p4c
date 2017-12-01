control ctrl() {
    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;
        a = 32w0;
        b = 32w1;
        c = 32w2;
        if (a == 32w0) {
            b = 32w2;
            return;
            c = 32w3;
        }
        else {
            b = 32w3;
            return;
            c = 32w4;
        }
        c = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

