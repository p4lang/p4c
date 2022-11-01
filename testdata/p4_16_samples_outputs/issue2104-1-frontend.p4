#include <core.p4>

control c(inout bit<8> a) {
    @name("c.x_0") bit<8> x;
    apply {
        x = a;
        a = x;
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
