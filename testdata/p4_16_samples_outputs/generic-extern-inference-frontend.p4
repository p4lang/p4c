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
    @name("Parser.tmp") bit<16> tmp;
    @name("Parser.tmp_0") bit<16> tmp_0;
    @name("Parser.tmp_1") bit<16> tmp_1;
    @name("Parser.tmp_2") bit<16> tmp_2;
    @name("Parser.tmp_3") bit<16> tmp_3;
    @name("Parser.tmp_4") bit<16> tmp_4;
    @name("Parser.tmp_5") bit<16> tmp_5;
    @name("Parser.plain") PlainExtern(16w1) plain_0;
    @name("Parser.inferred") ParametrizedExtern<bit<16>>(16w1) inferred_0;
    @name("Parser.specialized") ParametrizedExtern<bit<8>>(8w2) specialized_0;
    @name("Parser.aliased") ParametrizedExtern32(32w3) aliased_0;
    @name("Parser.inferredArray") ParametrizedExtern<bit<16>>[2](16w4) inferredArray_0;
    @name("Parser.specializedArray") ParametrizedExtern<bit<16>>[2](16w5) specializedArray_0;
    @name("Parser.ctrl") OtherExtern() ctrl_0;
    state start {
        ctrl_0.apply_any<PlainExtern>(plain_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<16>>>(inferred_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<8>>>(specialized_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<32>>>(aliased_0);
        tmp_1 = inferred_0.get();
        tmp_0 = tmp_1;
        tmp_2 = inferredArray_0[0].get();
        tmp_3 = tmp_0 + tmp_2;
        tmp = tmp_3;
        tmp_4 = specializedArray_0[1].get();
        tmp_5 = tmp + tmp_4;
        result = tmp_5;
        transition accept;
    }
}

control Control(inout bit<16> result) {
    @name("Control.tmp_6") bit<16> tmp_6;
    @name("Control.tmp_7") bit<16> tmp_7;
    @name("Control.tmp_8") bit<16> tmp_8;
    @name("Control.abstractInferred") AbstractExtern<bit<16>>(16w6) abstractInferred_0 = {
        void handle(in bit<16> v) {
            result = result + v;
        }
    };
    apply {
        tmp_6 = result;
        tmp_7 = abstractInferred_0.get();
        tmp_8 = tmp_6 + tmp_7;
        result = tmp_8;
    }
}

parser P(out bit<16> result);
control C(inout bit<16> result);
package Top(P p, C c);
Top(Parser(), Control()) main;
