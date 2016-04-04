struct S {
}

header hdr {
}

control p() {
    apply {
        int<2> a;
        int<4> c;
        bool d;
        bit<2> e;
        bit<4> f;
        hdr[5] stack;
        S g;
        S h;
        c = a[2];
        c = stack[d];
        f = e & f;
        d = g == h;
        d = d < d;
        d = a > c;
    }
}

