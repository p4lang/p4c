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
    @name("c.t1") bit<8>[16] t1_0;
    @name("c.t2") bit<8> t2_0;
    apply {
        t1_0 = hdr.data.arr[hdr.data.z].nested_arr;
        t2_0 = t1_0[hdr.data.w];
        hdr.data.arr[hdr.data.x].nested_arr[hdr.data.y] = t2_0;
    }
}

top<headers>(c()) main;
