extern e<T> {
    e();
    T get();
}

parser simple(in bit<2> a);
package m(simple n);
parser p1_0(in bit<2> a) {
    @name("w") bit<2> w;
    @name("tmp") bit<2> tmp_0;
    @name("ei") e<bit<2>>() ei;
    state start {
        tmp_0 = ei.get();
        w = tmp_0;
        transition accept;
    }
}

m(p1_0()) main;
