#include <core.p4>

extern e<T> {
    e();
    T get();
}

parser p1<T>(in T a) {
    e<T>() ei;
    state start {
        T w = ei.get();
        transition accept;
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;

