struct S {
    error b;
}

control C() {
    apply {
        S s8 = (S){b = 2};
    }
}

