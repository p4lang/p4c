struct S {
    bit<1> a;
    bit<1> b;
}

control c(out bit<1> b) {
    S s_0;
    apply {
        s_0 = { 1w0, 1w1 };
        s_0 = S {a = s_0.b,b = s_0.a};
        b = s_0.a;
    }
}

control e<T>(out T t);
package top<T>(e<T> e);
top<bit<1>>(c()) main;

