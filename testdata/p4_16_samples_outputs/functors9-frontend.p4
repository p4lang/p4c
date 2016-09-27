extern e<T> {
    e();
    T get();
}

parser p1<T>(in T a) {
    @name("w") T w_0;
    @name("ei") e<T>() ei_0;
    T tmp;
    state start {
        tmp = ei_0.get();
        w_0 = tmp;
        transition accept;
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1<bit<2>>()) main;
