control C();
package S(C c);
T f<T>(T x) {
    return x;
}
bit<8> f_0(bit<8> x) {
    return x;
}
control MyC() {
    apply {
        bit<8> y = f_0(8w255);
    }
}

S(MyC()) main;
