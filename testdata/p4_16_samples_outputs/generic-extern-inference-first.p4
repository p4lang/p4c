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

typedef ParametrizedExtern<bit<32>> ParametrizedExtern32;
parser Parser(out bit<16> result) {
    PlainExtern(16w1) plain;
    ParametrizedExtern<bit<16>>(16w1) inferred;
    ParametrizedExtern<bit<8>>(8w2) specialized;
    ParametrizedExtern32(32w3) aliased;
    ParametrizedExtern<bit<16>>[2](16w4) inferredArray;
    ParametrizedExtern<bit<16>>[2](16w5) specializedArray;
    OtherExtern() ctrl;
    state start {
        ctrl.apply_any<PlainExtern>(plain);
        ctrl.apply_any<ParametrizedExtern<bit<16>>>(inferred);
        ctrl.apply_any<ParametrizedExtern<bit<8>>>(specialized);
        ctrl.apply_any<ParametrizedExtern<bit<32>>>(aliased);
        result = inferred.get() + inferredArray[0].get() + specializedArray[1].get();
        transition accept;
    }
}

control Control(inout bit<16> result) {
    AbstractExtern<bit<16>>(16w6) abstractInferred = {
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
