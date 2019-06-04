struct T {
    bit<1> f;
}

struct S {
    tuple<T, T> f1;
    T           f2;
    bit<1>      z;
}

struct tuple_0 {
    T field;
    T field_0;
}

extern void f<T>(in T data);
control c(inout bit<1> r) {
    apply {
        S s = { { { 1w0 }, { 1w1 } }, { 1w0 }, 1w1 };
        f<tuple<T, T>>(s.f1);
        f<tuple_0>(tuple_0 {field = T {f = 1w0},field_0 = T {f = 1w1}});
        r = s.f2.f & s.z;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

