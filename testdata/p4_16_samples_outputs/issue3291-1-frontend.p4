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
    @name("c.hasReturned_0") bool hasReturned;
    @name("c.retval_0") bool retval;
    @name("c.tmp") bool tmp;
    @name("c.a") h_0 a;
    @name("c.a_0") h_0 a_2;
    @name("c.hasReturned") bool hasReturned_0;
    @name("c.retval") bool retval_0;
    @name("c.v") h_1 v_0;
    apply {
        hasReturned = false;
        a = (h_0){f = 1w0};
        a_2 = a;
        hasReturned_0 = false;
        v_0.f = a_2;
        hasReturned_0 = true;
        retval_0 = v_0.f == a_2;
        tmp = retval_0;
        hasReturned = true;
        retval = tmp;
        x = retval;
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;

