extern AbstractExtern<AbsT> {
    AbstractExtern(AbsT arg);
    abstract void handle(in AbsT v);
}

control c() {
    AbstractExtern(16w1) wrongAbstractType = {
        void handle(in bit<8> v) {
        }
    };
    apply {
    }
}

