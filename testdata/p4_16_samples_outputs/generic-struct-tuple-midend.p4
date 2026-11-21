struct tuple_0<T0, T1> {
    T0 f0;
    T1 f1;
}

struct tuple_0_bit32_bit32 {
    bit<32> f0;
    bit<32> f1;
}

struct S<T> {
    tuple_0<T, T> t;
}

struct S_bit32 {
    tuple_0_bit32_bit32 t;
}

