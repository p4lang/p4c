struct Headers {
    bit<8> a;
}

bit<8> f(inout bit<8> x, in bit<8> z) {
    return 8w4;
}
bit<8> g(inout bit<8> z) {
    z = 8w3;
    return 8w1;
}
control ingress(inout Headers h) {
    apply {
        f(h.a, g(h.a));
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top(ingress()) main;

