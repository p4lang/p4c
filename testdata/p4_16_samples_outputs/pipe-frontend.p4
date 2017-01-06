match_kind {
    ternary,
    exact
}

typedef bit<9> BParamType;
struct TArg1 {
    bit<9> field1;
    bool   drop;
}

struct TArg2 {
    int<32> field2;
}

struct PArg1 {
    bit<32> f0;
    bool    drop;
}

struct QArg1 {
    bit<32> f1;
}

struct QArg2 {
    bit<32> f2;
}

extern bs {
}

struct Packet_data {
}

action NoAction() {
}
control P_pipe_0(inout TArg1 pArg1, inout TArg2 pArg2) {
    TArg1 tmp;
    TArg2 tmp_0;
    TArg1 tmp_1;
    TArg2 tmp_2;
    @name("B_action") action B_action_0(out bit<9> barg, BParamType bData) {
        barg = (bit<9>)bData;
    }
    @name("C_action") action C_action_0(bit<9> cData) {
    }
    @name("T") table T_0(inout TArg1 tArg1, in TArg2 aArg2) {
        key = {
            tArg1.field1: ternary;
            aArg2.field2: exact;
        }
        actions = {
            B_action_0(tArg1.field1);
            C_action_0();
        }
        size = 32w5;
        const default_action = C_action_0(9w5);
    }
    @name("Drop") action Drop_0() {
        pArg1.drop = true;
    }
    @name("Tinner") table Tinner_0() {
        key = {
            pArg1.field1: ternary;
        }
        actions = {
            Drop_0();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        tmp = pArg1;
        tmp_0 = pArg2;
        T_0.apply(tmp, tmp_0);
        pArg1 = tmp;
        tmp_1 = pArg1;
        tmp_2 = pArg2;
        T_0.apply(tmp_1, tmp_2);
        pArg1 = tmp_1;
        Tinner_0.apply();
    }
}

control Q_pipe(inout TArg1 qArg1, inout TArg2 qArg2) {
    TArg1 tmp_3;
    TArg2 tmp_4;
    @name("p1") P_pipe_0() p1_0;
    apply {
        tmp_3 = qArg1;
        tmp_4 = qArg2;
        p1_0.apply(tmp_3, tmp_4);
        qArg1 = tmp_3;
        qArg2 = tmp_4;
    }
}

parser prs(bs b, out Packet_data p);
control pp(inout TArg1 arg1, inout TArg2 arg2);
package myswitch(prs prser, pp pipe);
parser my_parser(bs b, out Packet_data p) {
    state start {
        transition accept;
    }
}

myswitch(my_parser(), Q_pipe()) main;
