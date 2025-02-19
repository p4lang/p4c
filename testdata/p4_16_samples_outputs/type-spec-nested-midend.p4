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

control c(inout bit<8> a) {
    apply {
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
