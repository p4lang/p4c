extern bit<32> f(in bit<32> x, out bit<16> y);
control c(out bit<32> r) {
    @name("c.arg") bit<16> arg;
    @name("c.retval") bit<32> retval;
    @name("c.tmp") bit<32> tmp;
    @name("c.inlinedRetval") bit<32> inlinedRetval_0;
    apply {
        tmp = f(x = 32w1, y = arg);
        retval = tmp;
        inlinedRetval_0 = retval;
        r = inlinedRetval_0;
    }
}

control _c(out bit<32> r);
package top(_c _c);
top(c()) main;
