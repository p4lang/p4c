header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
control c(out bit<32> x, out bit<32> y) {
    @name("c.h") hdr h_0;
    @name("c.b") bool b_0;
    @name("c.a_0") bit<32> a_10;
    @name("c.hasReturned_0") bool hasReturned;
    @name("c.retval_0") bit<32> retval;
    @name("c.a_1") bit<32> a_11;
    @name("c.hasReturned") bool hasReturned_0;
    @name("c.retval") bit<32> retval_0;
    @name("c.a_2") bit<32> a_12;
    @name("c.hasReturned_0") bool hasReturned_3;
    @name("c.retval_0") bit<32> retval_3;
    @name("c.a_3") bit<32> a_13;
    @name("c.hasReturned") bool hasReturned_4;
    @name("c.retval") bit<32> retval_4;
    @name("c.a_4") bit<32> a_14;
    @name("c.hasReturned") bool hasReturned_5;
    @name("c.retval") bit<32> retval_5;
    @name("c.a_5") bit<32> a_15;
    @name("c.hasReturned_0") bool hasReturned_6;
    @name("c.retval_0") bit<32> retval_6;
    @name("c.a_6") bit<32> a_16;
    @name("c.hasReturned_0") bool hasReturned_7;
    @name("c.retval_0") bit<32> retval_7;
    @name("c.a_7") bit<32> a_17;
    @name("c.hasReturned") bool hasReturned_8;
    @name("c.retval") bit<32> retval_8;
    @name("c.a_8") bit<32> a_18;
    @name("c.hasReturned_0") bool hasReturned_9;
    @name("c.retval_0") bit<32> retval_9;
    @name("c.a_9") bit<32> a_19;
    @name("c.hasReturned") bool hasReturned_10;
    @name("c.retval") bit<32> retval_10;
    @name("c.simple_action") action simple_action() {
        a_10 = x;
        hasReturned = false;
        hasReturned = true;
        retval = a_10;
        y = retval;
        hasReturned_0 = false;
        a_11 = 32w0;
        hasReturned_0 = true;
        retval_0 = a_11;
        x = a_11;
        a_12 = x;
        hasReturned_3 = false;
        hasReturned_3 = true;
        retval_3 = a_12;
    }
    apply {
        h_0.setValid();
        h_0 = (hdr){a = 32w0};
        b_0 = h_0.isValid();
        h_0.setValid();
        if (b_0) {
            x = h_0.a;
            hasReturned_4 = false;
            a_13 = 32w0;
            hasReturned_4 = true;
            retval_4 = a_13;
            h_0.a = a_13;
            hasReturned_5 = false;
            a_14 = 32w0;
            hasReturned_5 = true;
            retval_5 = a_14;
            h_0.a = a_14;
            a_15 = h_0.a;
            hasReturned_6 = false;
            hasReturned_6 = true;
            retval_6 = a_15;
            a_16 = h_0.a;
            hasReturned_7 = false;
            hasReturned_7 = true;
            retval_7 = a_16;
        } else {
            hasReturned_8 = false;
            a_17 = 32w0;
            hasReturned_8 = true;
            retval_8 = a_17;
            h_0.a = a_17;
            x = retval_8;
            a_18 = h_0.a;
            hasReturned_9 = false;
            hasReturned_9 = true;
            retval_9 = a_18;
            x = retval_9;
            hasReturned_10 = false;
            a_19 = 32w0;
            hasReturned_10 = true;
            retval_10 = a_19;
            h_0.a = a_19;
        }
        simple_action();
    }
}

top(c()) main;

