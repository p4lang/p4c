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

control Q_pipe(inout TArg1 qArg1, inout TArg2 qArg2) {
    TArg1 pArg1_0;
    TArg2 pArg2_0;
    TArg1 tArg1_0;
    TArg2 aArg2_0;
    action NoAction_1() {
    }
    @name("p1.B_action") action p1_B_action_0(out bit<9> barg_0, BParamType bData) {
        barg_0 = (bit<9>)bData;
    }
    @name("p1.C_action") action p1_C_action_0(bit<9> cData) {
        pArg1_0.field1 = cData;
    }
    @name("p1.T") table p1_T() {
        key = {
            tArg1_0.field1: ternary;
            aArg2_0.field2: exact;
        }
        actions = {
            p1_B_action_0(tArg1_0.field1);
            p1_C_action_0;
        }
        size = 32w5;
        const default_action = p1_C_action_0(9w5);
    }
    @name("p1.Drop") action p1_Drop_0() {
        pArg1_0.drop = true;
    }
    @name("p1.Tinner") table p1_Tinner() {
        key = {
            pArg1_0.field1: ternary;
        }
        actions = {
            p1_Drop_0;
            NoAction_1;
        }
        const default_action = NoAction_1;
    }
    action act() {
        pArg1_0 = qArg1;
        pArg2_0 = qArg2;
        tArg1_0 = pArg1_0;
        aArg2_0 = pArg2_0;
    }
    action act_0() {
        pArg1_0 = tArg1_0;
        tArg1_0 = pArg1_0;
        aArg2_0 = pArg2_0;
    }
    action act_1() {
        pArg1_0 = tArg1_0;
    }
    action act_2() {
        qArg1 = pArg1_0;
        qArg2 = pArg2_0;
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
    table tbl_act_1() {
        actions = {
            act_1;
        }
        const default_action = act_1();
    }
    table tbl_act_2() {
        actions = {
            act_2;
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        p1_T.apply();
        tbl_act_0.apply();
        p1_T.apply();
        tbl_act_1.apply();
        p1_Tinner.apply();
        tbl_act_2.apply();
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
