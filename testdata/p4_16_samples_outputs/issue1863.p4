struct S {
    bit<1> a;
    bit<1> b;
}

control c(out bit<1> b) {
    apply {
        S s = { 0, 1 };
        s = { s.b, s.a };
        b = s.a;
    }
}

control e<T>(out T t);
package top<T>(e<T> e);
top(c()) main;

