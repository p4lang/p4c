#include <core.p4>

bit<8> test(inout bit<8> x) {
    return x;
}
control c(inout bit<8> a) {
    apply {
        test(a);
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
