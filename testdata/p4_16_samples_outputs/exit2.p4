control ctrl(out bit<32> c) {
    bit<32> x;
    action e() {
        exit;
        x = 1;
    }
    apply {
        bit<32> a;
        bit<32> b;
        a = 0;
        b = 1;
        c = 2;
        if (a == 0) {
            b = 2;
            e();
            c = 3;
        } else {
            b = 3;
            e();
            c = 4;
        }
        c = 5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

