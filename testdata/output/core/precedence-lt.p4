extern T f<T>(T x);
extern Object {
    T foo<T>();
}

struct data {
    bit<8> f;
    bit<8> foo;
}

control C(inout data d, inout bit<16> foo, Object o) {
    apply {
        if (4 + d.f < 10) {
            d.foo = (bit<8>)(o.foo<bit<16>>());
        }
    }
}

