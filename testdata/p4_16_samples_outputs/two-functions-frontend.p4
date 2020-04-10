control c(inout bit<8> a) {
    apply {
        {
            bit<8> x_0 = a;
            bool hasReturned = false;
            bit<8> retval;
            hasReturned = true;
            retval = x_0;
            a = x_0;
        }
        {
            bit<8> x_1 = a;
            bool hasReturned_0 = false;
            bit<8> retval_0;
            hasReturned_0 = true;
            retval_0 = x_1;
            a = x_1;
        }
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;

