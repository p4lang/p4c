extern bit<32> f(in bit<32> x, out bit<16> y);
control c(out bit<32> r) {
    @name("c.arg") bit<16> arg;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<32> retval;
    @name("c.tmp") bit<32> tmp;
    apply {
        hasReturned = false;
        tmp = f(x = 32w1, y = arg);
        hasReturned = true;
        retval = tmp;
        r = retval;
    }
}

control _c(out bit<32> r);
package top(_c _c);
top(c()) main;

