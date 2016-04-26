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
    @name("hasReturned") bool hasReturned;
    hasReturned = false;
}
control Q_pipe(inout TArg1 qArg1, inout TArg2 qArg2) {
    @name("pArg1_0") TArg1 pArg1_0_0;
    @name("pArg2_0") TArg2 pArg2_0_0;
    @name("p1.B_action") action p1_B_action(out bit<9> barg, BParamType bData) {
        barg = (bit<9>)bData;
    }
    @name("p1.C_action") action p1_C_action(bit<9> cData) {
        pArg1_0_0.field1 = cData;
    }
    @name("p1.T") table p1_T(inout TArg1 tArg1, in TArg2 aArg2) {
        key = {
            tArg1.field1: ternary;
            aArg2.field2: exact;
        }
        actions = {
            p1_B_action(tArg1.field1);
            p1_C_action;
        }
        size = 32w5;
        const default_action = p1_C_action(9w5);
    }
    @name("p1.Drop") action p1_Drop() {
        pArg1_0_0.drop = true;
    }
    @name("p1.Tinner") table p1_Tinner() {
        key = {
            pArg1_0_0.field1: ternary;
        }
        actions = {
            p1_Drop;
        }
        const default_action = NoAction;
    }
    action act() {
        pArg1_0_0 = qArg1;
        pArg2_0_0 = qArg2;
    }
    action act_0() {
        qArg1 = pArg1_0_0;
        qArg2 = pArg2_0_0;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        p1_T.apply(pArg1_0_0, pArg2_0_0);
        p1_T.apply(pArg1_0_0, pArg2_0_0);
        p1_Tinner.apply();
        tbl_act_0.apply();
    }
}

parser prs(in bs b, out Packet_data p);
control pp(inout TArg1 arg1, inout TArg2 arg2);
package myswitch(prs prser, pp pipe);
parser my_parser(in bs b, out Packet_data p) {
    state start {
    }
}

myswitch(my_parser(), Q_pipe()) main;
