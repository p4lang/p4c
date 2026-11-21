#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct mystruct1 {
    bit<4> a;
    bit<4> b;
}

struct mystruct2 {
    mystruct1 foo;
    bit<4>    a;
    bit<4>    b;
}

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header MyHeader1 {
    bit<8>    x1;
    int<8>    x2;
    bit<48>   x3;
    int<32>   x4;
    varbit<8> x5;
}

