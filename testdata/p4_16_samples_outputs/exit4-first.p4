control ctrl() {
    action e() {
        exit;
    }
    table t {
        actions = {
            e();
        }
        default_action = e();
    }
    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;
        a = 32w0;
        b = 32w1;
        c = 32w2;
        if (t.apply().hit) {
            b = 32w2;
            t.apply();
            c = 32w3;
        }
        else {
            b = 32w3;
            t.apply();
            c = 32w4;
        }
        c = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

