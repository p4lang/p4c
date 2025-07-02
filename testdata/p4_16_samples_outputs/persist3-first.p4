#include <core.p4>

@command_line("-O0") struct metadata_t {
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

void a1(in bit<32> x, persistent<bit<32>> y) {
    y = x;
}
void a2(in pair x, persistent<pair> y) {
    y.a = x.b;
}
void b1(out bit<32> x, in bit<32> y) {
    x = y;
}
void b2(out pair x, in pair y) {
    x = y;
}
void b3(inout bit<32> x, in bit<32> y) {
    x += y;
}
control c(inout headers_t hdr, inout metadata_t meta) {
    persistent<pair>[16]() data;
    persistent<pair>() a;
    persistent<bit<32>>() b;
    apply {
        if (hdr.h1.h1 > hdr.h1.h2) {
            a1(a.a, b);
            b2(data[hdr.h1.b2[3:0]], data[hdr.h1.b2[7:4]]);
            b3(b, a.b);
        } else {
            a2(data[hdr.h1.b1[3:0]], data[hdr.h1.b1[7:4]]);
            b1(b, a.b);
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, metadata_t>(c()) main;
