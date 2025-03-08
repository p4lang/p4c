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
    @name("c.inlinedRetval") bool inlinedRetval_1;
    @name("c.inlinedRetval_0") bool inlinedRetval_2;
    apply {
        a = (h_0){f = 1w0};
        a_2 = a;
        v_0.f = a_2;
        retval_0 = v_0.f == a_2;
        inlinedRetval_1 = retval_0;
        tmp = inlinedRetval_1;
        retval = tmp;
        inlinedRetval_2 = retval;
        x = inlinedRetval_2;
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
