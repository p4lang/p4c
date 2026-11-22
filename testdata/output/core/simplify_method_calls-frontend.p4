header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
control c(out bit<32> x, out bit<32> y) {
    @name("c.h") hdr h_0;
    @name("c.b") bool b_0;
    @name("c.a_0") bit<32> a_10;
    @name("c.retval_0") bit<32> retval;
    @name("c.inlinedRetval") bit<32> inlinedRetval_9;
    @name("c.a_1") bit<32> a_11;
    @name("c.a_7") bit<32> a_17;
    @name("c.a_8") bit<32> a_18;
    @name("c.retval_0") bit<32> retval_9;
    @name("c.inlinedRetval_7") bit<32> inlinedRetval_17;
    @name("c.simple_action") action simple_action() {
        a_10 = x;
        retval = a_10;
        inlinedRetval_9 = retval;
        y = inlinedRetval_9;
        a_11 = 32w0;
        x = a_11;
    }
    apply {
        h_0.setValid();
        h_0 = (hdr){a = 32w0};
        b_0 = h_0.isValid();
        h_0.setValid();
        if (b_0) {
            x = h_0.a;
        } else {
            a_17 = 32w0;
            h_0.a = a_17;
            a_18 = h_0.a;
            retval_9 = a_18;
            inlinedRetval_17 = retval_9;
            x = inlinedRetval_17;
        }
        simple_action();
    }
}

top(c()) main;
