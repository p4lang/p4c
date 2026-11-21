package Inner();
struct S<T> {
    bit<8> x;
}

const S<Inner> s = {x = 0};
