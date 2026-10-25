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

extern persistent<T> {
    persistent();
    T read();
    void write(in T value);
}

control c(inout headers_t hdr, inout metadata_t meta) {
    @name("c.data") persistent<bit<32>>[256]() data_0;
    apply {
        if (hdr.h1.h1 > hdr.h1.h2) {
            data_0[hdr.h1.b1].write(hdr.h1.f1);
        } else {
            hdr.h1.f2 = data_0[hdr.h1.b1].read();
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, metadata_t>(c()) main;
