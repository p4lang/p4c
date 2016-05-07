extern e<T> {
    T get();
}

parser p1<T>(in T a) {
    T w_0;
    e<T>() @name("ei") ei_0;
    state start {
        w_0 = ei_0.get();
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;
