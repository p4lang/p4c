struct S<T> {
    tuple<T, T> t;
}

struct S_ {
    tuple<_, _> t;
}

const S_ x = (S_){t = { 0, 0 }};
