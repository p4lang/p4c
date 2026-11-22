enum A {
    X,
    Y
}

enum bit<32> X {
    Z = 32w0,
    W = 32w3
}

match_kind {
    new_one
}

@annotation(2 , 3 ,) @annotation2[k="v"] header H {
    bit<32> f;
}

void f(out H h) {
    h = (H){f = 32w20};
    h = (H){f = 32w10};
}
