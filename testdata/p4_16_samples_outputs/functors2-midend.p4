parser p2_0(out bit<2> z2) {
    @name("x1") bit<2> x1;
    @name("x2") bit<2> x2;
    @name("x3") bit<2> x3;
    @name("z1_0") bit<2> z1;
    @name("z1_1") bit<2> z1_3;
    @name("z1_2") bit<2> z1_4;
    @name("tmp") bit<2> tmp_3;
    @name("tmp_0") bit<2> tmp_4;
    @name("tmp_1") bit<2> tmp_5;
    @name("tmp_2") bit<2> tmp_6;
    state start {
        z1 = 2w0;
        x1 = z1;
        z1_3 = 2w1;
        x2 = z1_3;
        z1_4 = 2w2;
        x3 = z1_4;
        tmp_3 = 2w3;
        tmp_4 = tmp_3 | x1;
        tmp_5 = tmp_4 | x2;
        tmp_6 = tmp_5 | x3;
        z2 = tmp_6;
        transition accept;
    }
}

parser simple(out bit<2> z);
package m(simple n);
m(p2_0()) main;
