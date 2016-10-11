control ctrl() {
    bit<32> a_0;
    bit<32> b_0;
    bit<32> c_0;
    @name("e") action e_0() {
        exit;
    }
    @name("t") table t_0() {
        actions = {
            e_0();
        }
        default_action = e_0();
    }
    apply {
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
        if (t_0.apply().hit) {
            b_0 = 32w2;
            t_0.apply();
            c_0 = 32w3;
        }
        else {
            b_0 = 32w3;
            t_0.apply();
            c_0 = 32w4;
        }
        c_0 = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
