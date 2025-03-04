struct h<t> {
    t f;
}

struct h_bit1 {
    bit<1> f;
}

struct h_h_bit1 {
    h_bit1 f;
}

control c(out bool x) {
    @name("c.retval_0") bool retval;
    @name("c.tmp") bool tmp;
    @name("c.a") h_bit1 a;
    @name("c.a_0") h_bit1 a_2;
    @name("c.retval") bool retval_0;
    @name("c.v") h_h_bit1 v_0;
    apply {
        a = (h_bit1){f = 1w0};
        a_2 = a;
        v_0.f = a_2;
        retval_0 = v_0.f == a_2;
        tmp = retval_0;
        retval = tmp;
        x = retval;
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
