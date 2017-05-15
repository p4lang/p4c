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

struct Empty {
}

const T t = { 32s10, 32s20 };
const S s = { { 32s15, 32s25 }, { 32s10, 32s20 } };
const int<32> x = 32s10;
const int<32> y = 32s25;
const int<32> w = 32s10;
const T t1 = { 32s15, 32s25 };
const Empty e = {  };
