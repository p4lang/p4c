#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

extern void func<T>(in T x);

struct foobar {
    bit<32>	a;
    bit<32>     b;
}

header h1 {
    bit<32>     f1;
    bit<32>     f2;
}

struct headers_t {
    h1          h;
}

control c(inout headers_t hdrs) {
    apply {
        func({ a = hdrs.h.f1, b = hdrs.h.f2 });
    }
}

top(c()) main;
