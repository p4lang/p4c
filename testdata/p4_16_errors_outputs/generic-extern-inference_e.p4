extern Parametrized<ArgT> {
    Parametrized(ArgT arg);
    ArgT get();
}

extern TwoParameters<A, B> {
    TwoParameters(A arg);
    B get();
}

control c1() {
    TwoParameters(16w1) notInferable;
    apply {
    }
}

control c2() {
    Parametrized(1) noWidth;
    apply {
    }
}

