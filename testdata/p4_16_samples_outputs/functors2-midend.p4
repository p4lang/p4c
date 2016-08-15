parser p2_0(out bit<2> z2) {
    bit<2> x1_0;
    bit<2> x2_0;
    bit<2> x3_0;
    bit<2> z1_0;
    bit<2> z1_1;
    bit<2> z1_2;
    state start {
        transition p1_start;
    }
    state p1_start {
        z1_0 = 2w0;
        transition start_0;
    }
    state start_0 {
        x1_0 = z1_0;
        transition p1_start_0;
    }
    state p1_start_0 {
        z1_1 = 2w1;
        transition start_1;
    }
    state start_1 {
        x2_0 = z1_1;
        transition p1_start_1;
    }
    state p1_start_1 {
        z1_2 = 2w2;
        transition start_2;
    }
    state start_2 {
        x3_0 = z1_2;
        z2 = 2w3 | x1_0 | x2_0 | x3_0;
        transition accept;
    }
}

parser simple(out bit<2> z);
package m(simple n);
m(p2_0()) main;
