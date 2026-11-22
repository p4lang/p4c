#include <core.p4>

const int cst = ...;

enum E {
    A, B, C
};

enum bit<32> Z {
    A = 2,
    B = 3,
};

struct S {
    bit<32> b;
    bit<16> f;
}

struct G<T> {
    T b;
}

header H {
    bit<32> b;
    bit<16> f;
}

header I {
    bit<32> b;
}

header_union HU {
    H h;
    I i;
}

enum bit<8> myEnum { One = 1, Two = 2, Three = 3 }

enum myEnum1 {e1, e2, e3}

header H1 {
    int<8> i;
    bool   b;
    bit<14> l;
    bit<1> b1;
}

struct S1 {
    myEnum      val;
    myEnum1     val1;
    error       errorNoOne;
    H1          h;
    bit<1>              z;
    bool        b;
    int<8>      i;
}

extern void f<D>(in D data);

control C(out bit<32> x) {
    H[2] stack;
    HU[2] hustack;
    S1 sa;
    bit<1> tmp;

    apply {
        x = ...;
        bool b = ...;
        error e = ...;
        E e1 = ...;
        Z z = ...;
        S s0 = ...;
        S s1 = { ... };
        S s2 = { 2, ... };
        S s3 = { b = 2, ... };
        S s4 = { f = 2, ... };
        S s5 = { b = 2, f = 3, ... };
        S s6 = (S){ ... };
        S s7 = (S){ 2, ... };
        S s8 = (S){ b = 2, ... };
        S s9 = (S){ f = 2, ... };
        S s10 = (S){ b = 2, f = 3, ... };
        H h0 = ...;
        H h1 = { ... };
        H h2 = { 2, ... };
        H h3 = { b = 2, ... };
        H h4 = { f = 2, ... };
        H h5 = { b = 2, f = 3, ... };
        H h6 = (H){ ... };
        H h7 = (H){ 2, ... };
        H h8 = (H){ b = 2, ... };
        H h9 = (H){ f = 2, ... };
        H h10 = (H){ b = 2, f = 3, ... };
        HU hu = ...;
        stack = ...;
        stack = { h0, ... };
        stack = (H[2]){ (H){#}, ... };
        hustack = ...;
        hustack = { ... };
        hustack = { hu, ... };
        //G<bit<32>> g = ...;

        S1 sb = { b = false, h={3, true, ...}, ...};
        tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1> xx = { 0, true, ... };
        H1 hb = ...;
        sb.h = hb;
        sa = sb;
        f<tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1>>(xx);
        tmp = sa.z;
    }
}

control CT(out bit<32> x);
package top(CT _c);

top(C()) main;
