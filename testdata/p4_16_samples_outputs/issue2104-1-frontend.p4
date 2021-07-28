#include <core.p4>

control c(inout bit<8> a) {
    @name("c.x_0") bit<8> x;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<8> retval;
    apply {
        x = a;
        hasReturned = false;
        hasReturned = true;
        retval = x;
        a = x;
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;

