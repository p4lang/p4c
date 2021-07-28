control c(inout bit<8> a) {
    @name("c.x_0") bit<8> x;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<8> retval;
    @name("c.x_1") bit<8> x_2;
    @name("c.hasReturned_0") bool hasReturned_0;
    @name("c.retval_0") bit<8> retval_0;
    apply {
        x = a;
        hasReturned = false;
        hasReturned = true;
        retval = x;
        a = x;
        x_2 = a;
        hasReturned_0 = false;
        hasReturned_0 = true;
        retval_0 = x_2;
        a = x_2;
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;

