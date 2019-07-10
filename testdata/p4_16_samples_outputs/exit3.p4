control ctrl(out bit<32> c) {
    action e() {
        exit;
    }
    table t {
        actions = {
            e;
        }
        default_action = e();
    }
    apply {
        bit<32> a;
        bit<32> b;
        a = 0;
        b = 1;
        c = 2;
        if (a == 0) {
            b = 2;
            t.apply();
            c = 3;
        } else {
            b = 3;
            t.apply();
            c = 4;
        }
        c = 5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

