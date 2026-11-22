#include <core.p4>

enum bit<8> myEnum { One = 1, Two = 2, Three = 3 }

enum myEnum1 {e1, e2, e3}


header H {
    int<8> i;
    bool   b;
    bit<14> l;
    bit<1> b1;
    varbit<8>   v;
}


struct S {
    myEnum      val;
    myEnum1     val1;
    error       errorNoOne;
    H           h;
    bit<1>              z;
    bool        b;
    int<8>      i;

}


extern void f<D>(in D data);
control c(inout bit<1> r) {
    S sa;
    bit<1> tmp;
    apply {
        S sb = { b = false, h={3, true, ..., 2 }, ...};
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
