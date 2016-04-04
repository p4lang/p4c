extern e<T> {
    T get();
}

parser p1<T>(in T a) {
    e<T>() ei;
    state start {
        T w = ei.get();
    }
}

parser p1_0(in bit<2> a) {
    e<bit<2>>() ei;
    state start {
        bit<2> w = ei.get();
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1_0()) main;
