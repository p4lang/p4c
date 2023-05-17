struct Headers {
    bit<8> a;
    bit<8> b;
}

bit<8> g(inout bit<8> z) {
    z = 8w3;
    return 8w1;
}

bit<8> f(in bit<8> z, inout bit<8> x) {
    return 8w4;
}

control ingress(inout Headers h) {
    apply {
        f(g(h.a), h.a);
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);

top(ingress()) main;
