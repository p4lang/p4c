struct S<T> {
    tuple<T, T> t;
}

struct S_0 {
    tuple<bit<32>, bit<32>> t;
}

const S_0 x = (S_0){t = { 32w0, 32w0 }};
