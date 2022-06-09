bit<1> h(in bit<1> a) {
    return a;
}
bit<1> f(in bit<1> a=h(1), in bit<1> b) {
    return b;
}
bit<1> g(in bit<1> a) {
    return f(b = a);
}
