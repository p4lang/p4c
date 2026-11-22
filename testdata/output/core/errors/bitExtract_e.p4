struct S {
}

control p() {
    apply {
        S s;
        bit<4> dt;
        bit<4> x = s[3:0];
        bit<8> y = dt[7:0];
        bit<4> z = dt[7:4];
    }
}

