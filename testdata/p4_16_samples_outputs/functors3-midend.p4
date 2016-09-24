parser p_0(out bit<1> z) {
    @name("z1_0") bit<1> z1;
    @name("tmp") bit<1> tmp_1;
    @name("tmp_0") bit<1> tmp_2;
    state start {
        z1 = 1w0;
        z = z1;
        tmp_1 = 1w0;
        tmp_2 = tmp_1 & 1w1;
        z = tmp_2;
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p_0()) main;
