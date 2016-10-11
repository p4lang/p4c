#include <core.p4>

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    bit<32> x_0;
    H tmp_0;
    bit<32> tmp;
    bit<32> tmp_1;
    state start {
        p.extract<H>(tmp_0);
        transition select(tmp_0.field) {
            32w0: n1;
            default: n2;
        }
    }
    state n1 {
        x_0 = 32w1;
        transition n3;
    }
    state n2 {
        x_0 = 32w2;
        transition n3;
    }
    state n3 {
        tmp = x_0 + 32w4294967295;
        x_0 = tmp;
        tmp_1 = x_0;
        p.extract<H>(h[tmp_1]);
        transition accept;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<H[2]>(P()) main;
