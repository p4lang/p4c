header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

header t2 {
    bit<32> x;
}

struct headers_t {
    t1    head;
    t2[8] stack;
}

