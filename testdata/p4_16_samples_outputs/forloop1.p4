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

control c(inout headers_t hdrs) {
    apply {
        for (t2 v in hdrs.stack) {
            hdrs.head.f1 = hdrs.head.f1 + v.x;
        }
        for (bit<8> v = 0, bit<16> x = 1; v < hdrs.head.cnt; v = v + 1, x = x << 1) {
            hdrs.head.h1 = hdrs.head.h1 + x;
        }
    }
}

