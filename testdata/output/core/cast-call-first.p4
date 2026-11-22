extern T f<T>(T x);
action a() {
    bit<32> x;
    x = (bit<32>)(f<bit<6>>(6w5));
}
