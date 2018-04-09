#include <core.p4>

extern e<T> {
    e();
    T get();
}

parser p1<T>(out T a) {
    e<T>() ei;
    state start {
        a = ei.get();
        transition accept;
    }
}

parser simple(out bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;

