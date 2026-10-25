struct S<T> {
    tuple<T, T> t;
}

const S<_> x = {t = { 0, 0 }};
