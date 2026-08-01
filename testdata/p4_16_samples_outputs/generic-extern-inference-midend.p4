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

parser Parser(out bit<16> result) {
    @name("Parser.tmp_1") bit<16> tmp_1;
    @name("Parser.tmp_2") bit<16> tmp_2;
    @name("Parser.tmp_4") bit<16> tmp_4;
    @name("Parser.plain") PlainExtern(16w1) plain_0;
    @name("Parser.inferred") ParametrizedExtern<bit<16>>(16w1) inferred_0;
    @name("Parser.specialized") ParametrizedExtern<bit<8>>(8w2) specialized_0;
    @name("Parser.aliased") ParametrizedExtern<bit<32>>(32w3) aliased_0;
    @name("Parser.inferredArray") ParametrizedExtern<bit<16>>[2](16w4) inferredArray_0;
    @name("Parser.specializedArray") ParametrizedExtern<bit<16>>[2](16w5) specializedArray_0;
    @name("Parser.ctrl") OtherExtern() ctrl_0;
    state start {
        ctrl_0.apply_any<PlainExtern>(plain_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<16>>>(inferred_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<8>>>(specialized_0);
        ctrl_0.apply_any<ParametrizedExtern<bit<32>>>(aliased_0);
        tmp_1 = inferred_0.get();
        tmp_2 = inferredArray_0[0].get();
        tmp_4 = specializedArray_0[1].get();
        result = tmp_1 + tmp_2 + tmp_4;
        transition accept;
    }
}

control Control(inout bit<16> result) {
    @name("Control.tmp_7") bit<16> tmp_7;
    @name("Control.abstractInferred") AbstractExtern<bit<16>>(16w6) abstractInferred_0 = {
        void handle(in bit<16> v) {
            result = result + v;
        }
    };
    @hidden action genericexterninference58() {
        tmp_7 = abstractInferred_0.get();
        result = result + tmp_7;
    }
    @hidden table tbl_genericexterninference58 {
        actions = {
            genericexterninference58();
        }
        const default_action = genericexterninference58();
    }
    apply {
        tbl_genericexterninference58.apply();
    }
}

parser P(out bit<16> result);
control C(inout bit<16> result);
package Top(P p, C c);
Top(Parser(), Control()) main;
