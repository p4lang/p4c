bit<1> f(in bit<1> t0) {
    return t0;
}
bit<2> f(in bit<2> t) {
    bit<1> b0;
    bit<1> b1;
    b0 = (bit<1>)t;
    b1 = (bit<1>)(t >> 1);
    b0 = f(b0);
    b1 = f(b1);
    bit<2> b = (bit<2>)b0;
    b = b | (bit<2>)b1 << 1;
    return b;
}
