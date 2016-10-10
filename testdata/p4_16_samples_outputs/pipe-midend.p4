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
    @name("pArg1_0") TArg1 pArg1;
    @name("pArg2_0") TArg2 pArg2;
    @name("tArg1") TArg1 tArg1_0;
    @name("aArg2") TArg2 aArg2_0;
    @name("tmp") TArg1 tmp_5;
    @name("tmp_0") TArg2 tmp_6;
    @name("tmp_1") TArg1 tmp_7;
    @name("tmp_2") TArg2 tmp_8;
    @name("tmp_3") TArg1 tmp_9;
    @name("tmp_4") TArg2 tmp_10;
    @name("barg") bit<9> barg_0;
    @name("NoAction_1") action NoAction() {
    }
    @name("p1.B_action") action p1_B_action(BParamType bData) {
        barg_0 = (bit<9>)bData;
        tArg1_0.field1 = (bit<9>)bData;
    }
    @name("p1.C_action") action p1_C_action(bit<9> cData) {
        pArg1.field1 = cData;
    }
    @name("p1.T") table p1_T_0() {
        key = {
            tArg1_0.field1: ternary;
            aArg2_0.field2: exact;
        }
        actions = {
            p1_B_action();
            p1_C_action();
        }
        size = 32w5;
        const default_action = p1_C_action(9w5);
    }
    @name("p1.Drop") action p1_Drop() {
        pArg1.drop = true;
    }
    @name("p1.Tinner") table p1_Tinner_0() {
        key = {
            pArg1.field1: ternary;
        }
        actions = {
            p1_Drop();
            NoAction();
        }
        const default_action = NoAction();
    }
    action act() {
        tmp_9 = qArg1;
        tmp_10 = qArg2;
        pArg1 = tmp_9;
        pArg2 = tmp_10;
        tmp_5 = pArg1;
        tmp_6 = pArg2;
        tArg1_0 = tmp_5;
        aArg2_0 = tmp_6;
    }
    action act_0() {
        tmp_5 = tArg1_0;
        pArg1 = tmp_5;
        tmp_7 = pArg1;
        tmp_8 = pArg2;
        tArg1_0 = tmp_7;
        aArg2_0 = tmp_8;
    }
    action act_1() {
        tmp_7 = tArg1_0;
        pArg1 = tmp_7;
    }
    action act_2() {
        tmp_9 = pArg1;
        tmp_10 = pArg2;
        qArg1 = tmp_9;
        qArg2 = tmp_10;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_1() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_2() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        p1_T_0.apply();
        tbl_act_0.apply();
        p1_T_0.apply();
        tbl_act_1.apply();
        p1_Tinner_0.apply();
        tbl_act_2.apply();
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
