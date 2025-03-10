struct S<T> {
    tuple<T, T> t;
}

struct S_bit32 {
    tuple<bit<32>, bit<32>> t;
}

const S_bit32 x = (S_bit32){t = { 32w0, 32w0 }};
