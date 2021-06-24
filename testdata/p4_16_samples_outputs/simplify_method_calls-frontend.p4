header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
control c(out bit<32> x, out bit<32> y) {
    @name("c.h") hdr h_0;
    @name("c.b") bool b_0;
    @name("c.a_0") bit<32> a_7;
    @name("c.hasReturned_0") bool hasReturned;
    @name("c.retval_0") bit<32> retval;
    @name("c.a_1") bit<32> a_8;
    @name("c.hasReturned") bool hasReturned_0;
    @name("c.retval") bit<32> retval_0;
    @name("c.a_2") bit<32> a_9;
    @name("c.hasReturned") bool hasReturned_3;
    @name("c.retval") bit<32> retval_3;
    @name("c.a_3") bit<32> a_10;
    @name("c.hasReturned") bool hasReturned_4;
    @name("c.retval") bit<32> retval_4;
    @name("c.a_4") bit<32> a_11;
    @name("c.hasReturned") bool hasReturned_5;
    @name("c.retval") bit<32> retval_5;
    @name("c.a_5") bit<32> a_12;
    @name("c.hasReturned_0") bool hasReturned_6;
    @name("c.retval_0") bit<32> retval_6;
    @name("c.a_6") bit<32> a_13;
    @name("c.hasReturned") bool hasReturned_7;
    @name("c.retval") bit<32> retval_7;
    @name("c.simple_action") action simple_action() {
        a_7 = x;
        hasReturned = false;
        hasReturned = true;
        retval = a_7;
        y = retval;
        hasReturned_0 = false;
        a_8 = 32w0;
        hasReturned_0 = true;
        retval_0 = a_8;
        x = a_8;
    }
    apply {
        h_0.setValid();
        h_0 = (hdr){a = 32w0};
        b_0 = h_0.isValid();
        h_0.setValid();
        if (b_0) {
            x = h_0.a;
            hasReturned_3 = false;
            a_9 = 32w0;
            hasReturned_3 = true;
            retval_3 = a_9;
            h_0.a = a_9;
            hasReturned_4 = false;
            a_10 = 32w0;
            hasReturned_4 = true;
            retval_4 = a_10;
            h_0.a = a_10;
        } else {
            hasReturned_5 = false;
            a_11 = 32w0;
            hasReturned_5 = true;
            retval_5 = a_11;
            h_0.a = a_11;
            x = retval_5;
            a_12 = h_0.a;
            hasReturned_6 = false;
            hasReturned_6 = true;
            retval_6 = a_12;
            x = retval_6;
            hasReturned_7 = false;
            a_13 = 32w0;
            hasReturned_7 = true;
            retval_7 = a_13;
            h_0.a = a_13;
        }
        simple_action();
    }
}

top(c()) main;

