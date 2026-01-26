#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

header t1 {
    bit<32>     f1;
    bit<32>     f2;
    bit<32>     f3;
    bit<32>     f4;
    bit<8>      b1;
    bit<8>      b2;
    int<8>      b3;
}

struct headers_t {
    t1          head;
}

extern bit<32> fn(in bit<32> x);

control c(inout headers_t hdrs) {
    action act(bit<32> x) { fn(x); }
    apply {
        if (hdrs.head.f1 == hdrs.head.f1) act(1);
        if (fn(2) == fn(2)) act(3);
        if (hdrs.head.f1 != hdrs.head.f1) act(4);
        if (hdrs.head.f1 >= hdrs.head.f1) act(5);
        if (hdrs.head.f1 > hdrs.head.f1) act(6);
        if (hdrs.head.f1 <= hdrs.head.f1) act(7);
        if (hdrs.head.f1 < hdrs.head.f1) act(8);

        if (hdrs.head.b1 >= 0) act(9);
        if (hdrs.head.b1 < 0) act(10);
        if (hdrs.head.b1 <= 255) act(11);
        if (hdrs.head.b1 > 255) act(12);
        if (hdrs.head.b3 >= -128) act(13);
        if (hdrs.head.b3 < -128) act(14);
        if (hdrs.head.b3 <= 127) act(15);
        if (hdrs.head.b3 > 127) act(16);

        if (0 <= hdrs.head.b1) act(17);
        if (0 > hdrs.head.b1) act(18);
        if (255 >= hdrs.head.b1) act(19);
        if (255 < hdrs.head.b1) act(20);
        if (-128 <= hdrs.head.b3) act(21);
        if (-128 > hdrs.head.b3) act(22);
        if (127 >= hdrs.head.b3) act(23);
        if (127 < hdrs.head.b3) act(24);
    }
}

top(c()) main;
