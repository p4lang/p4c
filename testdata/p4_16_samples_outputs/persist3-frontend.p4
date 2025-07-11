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
    x = x + y;
}
control c(inout headers_t hdr, inout metadata_t meta) {
    bit<4> tmp;
    bit<4> tmp_0;
    pair tmp_1;
    bit<4> tmp_2;
    @name("c.data") persistent<pair>[16]() data_0;
    @name("c.a") persistent<pair>() a_0;
    @name("c.b") persistent<bit<32>>() b_0;
    apply {
        if (hdr.h1.h1 > hdr.h1.h2) {
            a1(a_0.a, b_0);
            tmp = hdr.h1.b2[3:0];
            tmp_0 = hdr.h1.b2[7:4];
            tmp_1 = data_0[tmp_0];
            b2(data_0[tmp], tmp_1);
            b3(b_0, a_0.b);
        } else {
            tmp_2 = hdr.h1.b1[3:0];
            a2(data_0[tmp_2], data_0[hdr.h1.b1[7:4]]);
            b1(b_0, a_0.b);
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, metadata_t>(c()) main;
