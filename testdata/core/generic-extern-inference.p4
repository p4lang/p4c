// Type arguments of a generic extern type can be inferred from the constructor
// arguments, in which case the instantiation does not have to be specialized
// explicitly (see P4-16 specification, section "Type specialization").
// Such an instance can be used wherever an explicitly specialized one can be,
// in particular as the argument of a generic method.

extern PlainExtern {
    PlainExtern(bit<16> arg);
}

extern ParametrizedExtern<ArgT> {
    ParametrizedExtern(ArgT arg);
    ArgT get();
}

extern OtherExtern {
    OtherExtern();
    void apply_any<CatchAllT>(CatchAllT arg);
}

extern AbstractExtern<AbsT> {
    AbstractExtern(AbsT arg);
    abstract void handle(in AbsT v);
    AbsT get();
}

// A name which is not generic itself, but stands for a specialized generic type.
typedef ParametrizedExtern<bit<32>> ParametrizedExtern32;

parser Parser(out bit<16> result) {
    PlainExtern(16w1) plain;
    ParametrizedExtern(16w1) inferred;            // ArgT is inferred to be bit<16>
    ParametrizedExtern<bit<8>>(8w2) specialized;  // ArgT is supplied explicitly
    ParametrizedExtern32(32w3) aliased;
    ParametrizedExtern(16w4) inferredArray[2];
    ParametrizedExtern<bit<16>>(16w5) specializedArray[2];
    OtherExtern() ctrl;

    state start {
        ctrl.apply_any(plain);
        ctrl.apply_any(inferred);
        ctrl.apply_any(specialized);
        ctrl.apply_any(aliased);
        result = inferred.get() + inferredArray[0].get() + specializedArray[1].get();
        transition accept;
    }
}

control Control(inout bit<16> result) {
    // The abstract method is implemented for the inferred type argument bit<16>.
    AbstractExtern(16w6) abstractInferred = {
        void handle(in bit<16> v) {
            result = result + v;
        }
    };

    apply {
        result = result + abstractInferred.get();
    }
}

parser P(out bit<16> result);
control C(inout bit<16> result);
package Top(P p, C c);
Top(Parser(), Control()) main;
