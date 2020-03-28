#include <core.p4>

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
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;

