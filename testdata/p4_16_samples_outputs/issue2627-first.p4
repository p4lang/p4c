struct H3<T> {
    tuple<T> t;
}

struct S {
    bit<32> b;
}

struct H3_0 {
    tuple<S> t;
}

const H3_0 h4 = (H3_0){t = { (S){b = 32w0} }};
