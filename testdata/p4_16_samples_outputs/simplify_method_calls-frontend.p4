header hdr {
    bit<32> a;
}

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);
control c(out bit<32> x, out bit<32> y) {
    @name("c.h") hdr h_0;
    @name("c.b") bool b_0;
    @name("c.simple_action") action simple_action() {
        {
            @name("c.a_0") bit<32> a_0 = x;
            @name("c.hasReturned_0") bool hasReturned = false;
            @name("c.retval_0") bit<32> retval;
            hasReturned = true;
            retval = a_0;
            y = retval;
        }
        {
            @name("c.a_1") bit<32> a_1;
            @name("c.hasReturned") bool hasReturned_0 = false;
            @name("c.retval") bit<32> retval_0;
            a_1 = 32w0;
            hasReturned_0 = true;
            retval_0 = a_1;
            x = a_1;
        }
    }
    apply {
        h_0.setValid();
        h_0 = (hdr){a = 32w0};
        b_0 = h_0.isValid();
        h_0.setValid();
        if (b_0) {
            x = h_0.a;
            {
                @name("c.a_2") bit<32> a_2;
                @name("c.hasReturned") bool hasReturned_3 = false;
                @name("c.retval") bit<32> retval_3;
                a_2 = 32w0;
                hasReturned_3 = true;
                retval_3 = a_2;
                h_0.a = a_2;
            }
            {
                @name("c.a_3") bit<32> a_3;
                @name("c.hasReturned") bool hasReturned_4 = false;
                @name("c.retval") bit<32> retval_4;
                a_3 = 32w0;
                hasReturned_4 = true;
                retval_4 = a_3;
                h_0.a = a_3;
            }
        } else {
            {
                @name("c.a_4") bit<32> a_4;
                @name("c.hasReturned") bool hasReturned_5 = false;
                @name("c.retval") bit<32> retval_5;
                a_4 = 32w0;
                hasReturned_5 = true;
                retval_5 = a_4;
                h_0.a = a_4;
                x = retval_5;
            }
            {
                @name("c.a_5") bit<32> a_5 = h_0.a;
                @name("c.hasReturned_0") bool hasReturned_6 = false;
                @name("c.retval_0") bit<32> retval_6;
                hasReturned_6 = true;
                retval_6 = a_5;
                x = retval_6;
            }
            {
                @name("c.a_6") bit<32> a_6;
                @name("c.hasReturned") bool hasReturned_7 = false;
                @name("c.retval") bit<32> retval_7;
                a_6 = 32w0;
                hasReturned_7 = true;
                retval_7 = a_6;
                h_0.a = a_6;
            }
        }
        simple_action();
    }
}

top(c()) main;

