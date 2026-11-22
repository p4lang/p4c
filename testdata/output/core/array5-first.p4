control proto<P>(inout P pkt);
package top<P>(proto<P> p);
header data_t {
    bit<8>[2][2] arr;
    bit<1>       w;
    bit<1>       x;
    bit<1>       y;
    bit<1>       z;
}

struct headers {
    data_t data;
}

control c(inout headers hdr) {
    apply {
        hdr.data.arr[hdr.data.w][hdr.data.x] = hdr.data.arr[hdr.data.y][hdr.data.z];
    }
}

top<headers>(c()) main;
