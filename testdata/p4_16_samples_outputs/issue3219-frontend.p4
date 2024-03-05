control c(out bit<4> result) {
    @name("c.retval") bit<4> retval;
    @name("c.t") bit<4> t_0;
    apply {
        t_0 = 4w1;
        retval = t_0;
        result = retval;
    }
}

control _c(out bit<4> r);
package top(_c _c);
top(c()) main;
