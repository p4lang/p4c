extern e<T> {
    T get();
}

parser p1<T>(in T a) {
    state start {
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;
