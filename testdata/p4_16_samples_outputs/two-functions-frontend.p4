control c(inout bit<8> a) {
    apply {
        {
            @name("c.x_0") bit<8> x_0 = a;
            @name("c.hasReturned") bool hasReturned = false;
            @name("c.retval") bit<8> retval;
            hasReturned = true;
            retval = x_0;
            a = x_0;
        }
        {
            @name("c.x_1") bit<8> x_1 = a;
            @name("c.hasReturned_0") bool hasReturned_0 = false;
            @name("c.retval_0") bit<8> retval_0;
            hasReturned_0 = true;
            retval_0 = x_1;
            a = x_1;
        }
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;

