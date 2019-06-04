extern void f<T>(in T data);
struct S {
    bit<32> f;
}

struct S1 {
    bit<32> f;
    bit<32> g;
}

action a() {
    S x;
    S1 y;
    f<S>(S {f = 32w0});
    f<S>(x);
    f<S1>(y);
    f<S1>(S1 {f = 32w0,g = 32w1});
}
