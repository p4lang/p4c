#include <core.p4>

control c(inout bit<8> a) {
    apply {
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
