extern e<T> {
    e();
    T get();
}

parser simple(in bit<2> a);
package m(simple n);
parser p1_0(in bit<2> a) {
    bit<2> w_0;
    bit<2> tmp;
    @name("ei") e<bit<2>>() ei_0;
    state start {
        tmp = ei_0.get();
        w_0 = tmp;
        transition accept;
    }
}

m(p1_0()) main;
