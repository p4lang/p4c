#include <core.p4>

parser p1<T>(in T a) {
    state start {
        T w = a;
        transition accept;
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;

