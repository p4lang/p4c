struct P {
    bit<32> f1;
    bit<32> f2;
}

struct T {
    int<32> t1;
    int<32> t2;
}

struct S {
    T s1;
    T s2;
}

const T t = { 10, 20 };
const S s = { { 15, 25 }, t };
const int<32> x = t.t1;
const int<32> y = s.s1.t2;
const int<32> w = .t.t1;
const T t1 = s.s1;
