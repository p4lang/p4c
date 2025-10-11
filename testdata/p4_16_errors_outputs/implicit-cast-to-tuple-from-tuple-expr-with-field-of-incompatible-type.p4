struct S<T> {
    tuple<T, T> t;
}

const S<tuple<>> x = {t = { 0, 0 }};
