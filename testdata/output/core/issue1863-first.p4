struct S {
    bit<1> a;
    bit<1> b;
}

control c(out bit<1> b) {
    apply {
        S s = (S){a = 1w0,b = 1w1};
        s = (S){a = s.b,b = s.a};
        b = s.a;
    }
}

control e<T>(out T t);
package top<T>(e<T> e);
top<bit<1>>(c()) main;
