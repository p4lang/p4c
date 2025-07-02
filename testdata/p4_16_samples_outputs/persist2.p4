#include <core.p4>

struct metadata_t {
}

header h1_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

struct headers_t {
    h1_t h1;
}

struct pair {
    bit<32> a;
    bit<32> b;
}

extern persistent<T> {
    persistent();
    @implicit T read();
    @implicit void write(in T value);
}

control c(inout headers_t hdr, inout metadata_t meta) {
    persistent<pair>[16]() data;
    apply {
        if (hdr.h1.h1 > hdr.h1.h2) {
            data[hdr.h1.b1[3:0]].a = hdr.h1.f1;
            data[hdr.h1.b1[3:0]].b = hdr.h1.f2;
        } else {
            hdr.h1.f1 = data[hdr.h1.b1[3:0]].a;
            hdr.h1.f2 = data[hdr.h1.b1[3:0]].b;
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top(c()) main;
