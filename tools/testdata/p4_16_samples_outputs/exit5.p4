control ctrl() {
    action e() {
        exit;
    }
    action f() {
    }
    table t {
        actions = {
            e;
            f;
        }
        default_action = e();
    }
    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;
        a = 0;
        b = 1;
        c = 2;
        switch (t.apply().action_run) {
            e: {
                b = 2;
                t.apply();
                c = 3;
            }
            f: {
                b = 3;
                t.apply();
                c = 4;
            }
        }

        c = 5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

