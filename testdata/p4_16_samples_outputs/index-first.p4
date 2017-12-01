#include <core.p4>

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    bit<32> x;
    H tmp;
    state start {
        p.extract<H>(tmp);
        transition select(tmp.field) {
            32w0: n1;
            default: n2;
        }
    }
    state n1 {
        x = 32w1;
        transition n3;
    }
    state n2 {
        x = 32w2;
        transition n3;
    }
    state n3 {
        x = x + 32w4294967295;
        transition n4;
    }
    state n4 {
        p.extract<H>(h[x]);
        transition accept;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<H[2]>(P()) main;

