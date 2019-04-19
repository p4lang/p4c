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

typedef bit<32> PArg2;
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

control T_host(inout TArg1 tArg1, in TArg2 aArg2)(bit<32> t2Size) {
    action B_action(out bit<9> barg, BParamType bData) {
        barg = bData;
    }
    action C_action(bit<9> cData) {
        tArg1.field1 = cData;
    }
    table T {
        key = {
            tArg1.field1: ternary @name("tArg1.field1") ;
            aArg2.field2: exact @name("aArg2.field2") ;
        }
        actions = {
            B_action(tArg1.field1);
            C_action();
        }
        size = t2Size;
        const default_action = C_action(9w5);
    }
    apply {
        T.apply();
    }
}

control P_pipe(inout TArg1 pArg1, inout TArg2 pArg2)(bit<32> t2Size) {
    T_host(t2Size) thost;
    action Drop() {
        pArg1.drop = true;
    }
    table Tinner {
        key = {
            pArg1.field1: ternary @name("pArg1.field1") ;
        }
        actions = {
            Drop();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        thost.apply(pArg1, pArg2);
        thost.apply(pArg1, pArg2);
        Tinner.apply();
    }
}

control Q_pipe(inout TArg1 qArg1, inout TArg2 qArg2) {
    P_pipe(32w5) p1;
    apply {
        p1.apply(qArg1, qArg2);
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

