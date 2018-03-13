#include <core.p4>

extern widget<T> {
}

extern widget<T> createWidget<T, U>(U a, T b);
header hdr_t {
    bit<16> f1;
    bit<16> f2;
    bit<16> f3;
}

control c1<T>(inout hdr_t hdr)(widget<T> w) {
    apply {
    }
}

control c2(inout hdr_t hdr) {
    c1<bit<16>>(createWidget(hdr.f1, hdr.f2)) c;
    apply {
        c.apply(hdr);
    }
}

