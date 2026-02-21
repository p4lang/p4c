control proto<P>(inout P pkt);
package top<P>(proto<P> p);
enum bit<8> E1 {
    A = 8w1,
    B = 8w2,
    C = 8w10
}

enum bit<8> E2 {
    X = 8w2,
    Y = 8w1,
    Z = 8w3
}

header data_t {
    E1     e1;
    E2     e2;
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct headers {
    data_t data;
}

control c(inout headers hdr) {
    apply {
        if (hdr.data.e1 == hdr.data.e2) {
            hdr.data.a[0:0] = 1w1;
        }
        if (hdr.data.e1 == E1.A) {
            hdr.data.a[1:1] = 1w1;
        }
        if (hdr.data.e1 == E2.X) {
            hdr.data.a[2:2] = 1w1;
        }
        if (hdr.data.e1 == hdr.data.c) {
            hdr.data.a[3:3] = 1w1;
        }
        if (hdr.data.c == E1.B) {
            hdr.data.a[4:4] = 1w1;
        }
        if (hdr.data.c == E2.Y) {
            hdr.data.a[5:5] = 1w1;
        }
        if (E1.A == E2.X) {
            hdr.data.b[0:0] = 1w1;
        }
        if (E1.A == E2.Y) {
            hdr.data.b[1:1] = 1w1;
        }
    }
}

top<headers>(c()) main;
