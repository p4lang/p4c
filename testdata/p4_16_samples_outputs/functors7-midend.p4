parser p1_0(in bit<2> a) {
    state start {
        transition accept;
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1_0()) main;
