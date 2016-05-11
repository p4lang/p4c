struct S {
    bit<1> d;
}

const S c = { 1w1 };
control p() {
    apply {
        S a;
        a.d = 1w1;
    }
}

