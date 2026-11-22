#include <core.p4>

header H {
    bit<1> a;
}

struct headers {
    H h;
}

control c(inout headers hdr) {
    apply {
    }
}

control e<T>(inout T t);
package top<T>(e<T> e);
top<headers>(c()) main;
