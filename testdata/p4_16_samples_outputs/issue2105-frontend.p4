#include <core.p4>

header H {
    bit<8> a;
}

struct Headers {
    H h;
}

control c() {
    apply {
    }
}

control e<T>();
package top<T>(e<T> e);
top<_>(c()) main;
