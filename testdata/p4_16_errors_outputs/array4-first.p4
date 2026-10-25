control proto<P>(inout P pkt);
package top<P>(proto<P> p);
struct S {
    bit<16>    f1;
    bit<8>[16] nested_arr;
    bit<16>    f2;
}

header data_t {
    S[16]  arr;
    bit<4> w;
    bit<4> x;
    bit<4> y;
    bit<4> z;
}

struct headers {
    data_t data;
}

control c(inout headers hdr) {
    apply {
        bit<8>[16] t1 = hdr.data.arr[hdr.data.z].nested_arr;
        bit<8> t2 = t1[hdr.data.w];
        hdr.data.arr[hdr.data.x].nested_arr[hdr.data.y] = t2;
    }
}

top<headers>(c()) main;
