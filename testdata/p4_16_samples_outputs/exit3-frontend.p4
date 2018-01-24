control ctrl(out bit<32> c) {
    bit<32> a;
    @name("ctrl.e") action e_0() {
        exit;
    }
    @name("ctrl.t") table t {
        actions = {
            e_0();
        }
        default_action = e_0();
    }
    apply {
        a = 32w0;
        c = 32w2;
        if (a == 32w0) 
            t.apply();
        else 
            t.apply();
        c = 32w5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

