enum A {
    X,
    Y,
}

enum bit<32> X {
    Z = 0,
    W = 3,
}

match_kind {
    new_one,
}

@annotation(2, 3,)
@annotation2[k="v",]
header H {
    bit<32> f;
}

void f(out H h) {
    h = { 20, };
    h = { f = 10, };
}