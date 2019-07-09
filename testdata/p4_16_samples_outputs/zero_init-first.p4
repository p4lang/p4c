struct T {
    bit<1> f;
}

struct S {
    tuple<T, T>         f1;
    T                   f2;
    bit<1>              z;
    tuple<int<8>, bool> tpl;
}

header H {
    int<8> i;
    bool   b;
    bit<7> l;
    bit<7> l1;
    bit<1> b1;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    S s_0;
    H h_0;
    bit<1> tmp;
    apply {
        tuple<int<8>, bool> x = {  };
        s_0.tpl = x;
        s_0 = {  };
        H h_1 = {  };
        h_0 = {  };
        h_0.b1 = h_1.b1;
        s_0.z = h_0.b1;
        f<tuple<T, T>>(s_0.f1);
        tmp = s_0.f2.f & s_0.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
