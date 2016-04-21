extern e<T> {
    T get();
}

parser p1<T>(in T a) {
    @name("w") T w_0;
    e<T>() ei;
    state start {
        w_0 = ei.get();
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;
