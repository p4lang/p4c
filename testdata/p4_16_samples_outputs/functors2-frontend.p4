parser p1(out bit<2> z1)(bit<2> a) {
    state start {
        z1 = a;
        transition accept;
    }
}

parser p2(out bit<2> z2)(bit<2> b, bit<2> c) {
    @name("x1") bit<2> x1_0;
    @name("x2") bit<2> x2_0;
    @name("x3") bit<2> x3_0;
    @name("p1a") p1(2w0) p1a_0;
    @name("p1b") p1(b) p1b_0;
    @name("p1c") p1(c) p1c_0;
    bit<2> tmp;
    bit<2> tmp_0;
    bit<2> tmp_1;
    bit<2> tmp_2;
    state start {
        p1a_0.apply(x1_0);
        p1b_0.apply(x2_0);
        p1c_0.apply(x3_0);
        tmp = b | c;
        tmp_0 = tmp | x1_0;
        tmp_1 = tmp_0 | x2_0;
        tmp_2 = tmp_1 | x3_0;
        z2 = tmp_2;
        transition accept;
    }
}

parser simple(out bit<2> z);
package m(simple n);
m(p2(2w1, 2w2)) main;
