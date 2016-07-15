extern e<T> {
    T get();
}

parser p1_0(in bit<2> a) {
    bit<2> w_0;
    @name("ei") e<bit<2>>() ei_0;
    state start {
        w_0 = ei_0.get();
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1_0()) main;
