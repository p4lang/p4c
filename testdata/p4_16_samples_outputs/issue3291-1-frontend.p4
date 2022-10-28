struct h<t> {
    t f;
}

struct h_0 {
    bit<1> f;
}

struct h_1 {
    h_0 f;
}

control c(out bool x) {
    @name("c.retval_0") bool retval;
    @name("c.tmp") bool tmp;
    @name("c.a") h_0 a;
    @name("c.a_0") h_0 a_2;
    @name("c.retval") bool retval_0;
    @name("c.v") h_1 v_0;
    apply {
        a = (h_0){f = 1w0};
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
