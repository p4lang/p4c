void test<T>(in T val) {
}
struct S2_0 {
    int<6> x;
    int<6> y;
}

struct S2<T> {
    T x;
    T y;
}

struct S1_0 {
    bit<4> x;
    S2_0   y;
}

struct S1<T1, T2> {
    T1     x;
    S2<T2> y;
}

void test_0(in S1_0 val) {
}
control c(inout bit<8> a) {
    apply {
        test_0((S1_0){x = 4w0,y = (S2_0){x = 6s0,y = 6s0}});
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
