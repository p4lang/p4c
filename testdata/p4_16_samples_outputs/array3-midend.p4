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
    @hidden action array3l31() {
        hdr.data.arr[0].nested_arr[1] = hdr.data.arr[2].nested_arr[3];
    }
    @hidden table tbl_array3l31 {
        actions = {
            array3l31();
        }
        const default_action = array3l31();
    }
    apply {
        tbl_array3l31.apply();
    }
}

top<headers>(c()) main;
