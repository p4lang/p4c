struct S {
}

control C(inout tuple<> S) {
    apply {
    }
}

control C2() {
    apply {
        S s;
        C.apply(s);
    }
}

