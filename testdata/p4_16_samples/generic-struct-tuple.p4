struct S<T> {
    tuple<T, T> t;
}

const S<bit<32>> x = { t = { 0, 0 } };