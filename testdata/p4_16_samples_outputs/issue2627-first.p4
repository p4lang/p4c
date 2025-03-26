struct H3<T> {
    tuple<T> t;
}

struct S {
    bit<32> b;
}

struct H3_S {
    tuple<S> t;
}

const H3_S h4 = (H3_S){t = { (S){b = 32w0} }};
