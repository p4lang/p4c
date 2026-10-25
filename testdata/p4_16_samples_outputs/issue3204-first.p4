struct h<t> {
    t f;
}

struct h_bit1 {
    bit<1> f;
}

bit<1> func() {
    h_bit1 a;
    a.f = 1w1;
    return a.f;
}
