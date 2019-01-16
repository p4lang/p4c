#include <core.p4>

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
    @name(".NoAction") action NoAction_0() {
    }
    @name("Q_pipe.p1.thost.B_action") action p1_thost_B_action_0(BParamType bData) {
        qArg1.field1 = bData;
    }
    @name("Q_pipe.p1.thost.C_action") action p1_thost_C_action_0(bit<9> cData) {
        qArg1.field1 = cData;
    }
    @name("Q_pipe.p1.thost.T") table p1_thost_T {
        key = {
            qArg1.field1: ternary @name("tArg1.field1") ;
            qArg2.field2: exact @name("aArg2.field2") ;
        }
        actions = {
            p1_thost_B_action_0();
            p1_thost_C_action_0();
        }
        size = 32w5;
        const default_action = p1_thost_C_action_0(9w5);
    }
    @name("Q_pipe.p1.Drop") action p1_Drop_0() {
        qArg1.drop = true;
    }
    @name("Q_pipe.p1.Tinner") table p1_Tinner {
        key = {
            qArg1.field1: ternary @name("pArg1.field1") ;
        }
        actions = {
            p1_Drop_0();
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    apply {
        p1_thost_T.apply();
        p1_thost_T.apply();
        p1_Tinner.apply();
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

