// The type arguments of a generic extern type may be omitted only if the compiler can infer
// them from the constructor arguments.
// The positive cases are in testdata/p4_16_samples/generic-extern-inference.p4.

extern Parametrized<ArgT> {
    Parametrized(ArgT arg);
    ArgT get();
}

extern TwoParameters<A, B> {
    TwoParameters(A arg);
    B get();
}

control c1() {
    // B does not appear among the constructor parameters, so it cannot be inferred.
    TwoParameters(16w1) notInferable;
    apply {}
}

control c2() {
    // The argument is an unsized integer constant, so no width can be inferred for ArgT.
    Parametrized(1) noWidth;
    apply {}
}
