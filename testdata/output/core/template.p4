extern bit<32> f<T>(in T data);
action a() {
    bit<8> x;
    bit<4> y;
    bit<32> r = f(x) + f(y);
}
