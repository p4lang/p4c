control ctrl(out bit<32> c) {
    bit<32> a_0;
    @name("ctrl.e") action e() {
        exit;
    }
    @name("ctrl.e") action e_2() {
        exit;
    }
    apply {
        a_0 = 32w0;
        c = 32w2;
        if (a_0 == 32w0) {
            e();
        } else {
            e_2();
        }
        c = 32w5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

