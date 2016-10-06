#include <core.p4>

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    @name("x") bit<32> x;
    @name("tmp") H tmp_2;
    @name("tmp") bit<32> tmp_3;
    @name("tmp_1") bit<32> tmp_4;
    state start {
        p.extract<H>(tmp_2);
        transition select(tmp_2.field) {
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
        tmp_3 = x + 32w4294967295;
        x = tmp_3;
        tmp_4 = x;
        p.extract<H>(h[tmp_4]);
        transition accept;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<H[2]>(P()) main;
