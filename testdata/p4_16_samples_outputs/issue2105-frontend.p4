#include <core.p4>
#include <v1model.p4>

header H {
    bit<8> a;
}

struct Headers {
    H h;
}

control c() {
    bit<8> x_0;
    apply {
        x_0 = 8w0;
    }
}

control e<T>();
package top<T>(e<T> e);
top<_>(c()) main;

