// The abstract methods of a generic extern type are checked against the type arguments which
// were inferred from the constructor arguments.

extern AbstractExtern<AbsT> {
    AbstractExtern(AbsT arg);
    abstract void handle(in AbsT v);
}

control c() {
    // AbsT is inferred to be bit<16>, so the implementation must not take a bit<8>.
    AbstractExtern(16w1) wrongAbstractType = {
        void handle(in bit<8> v) {}
    };
    apply {}
}
