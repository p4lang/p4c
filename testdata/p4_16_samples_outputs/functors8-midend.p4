#include <core.p4>

extern e<T> {
    e();
    T get();
}

parser simple(out bit<2> a);
package m(simple n);
parser p1_0(out bit<2> a) {
    bit<2> tmp_0;
    @name("p1_0.ei") e<bit<2>>() ei;
    state start {
        tmp_0 = ei.get();
        a = tmp_0;
        transition accept;
    }
}

m(p1_0()) main;

