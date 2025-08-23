#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
}

struct headers_t {
    t1 head;
}

typedef bit<1> Foo_t;
enum Foo_t Foo {
    A = 1w0,
    B = 1w1
}

extern Object {
    Object(int size);
    void set(Foo t);
}

Object(10) obj;
control c(inout headers_t hdrs) {
    apply {
        obj.set(1w1);
    }
}

top(c()) main;
