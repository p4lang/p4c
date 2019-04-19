#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef int<32> MySignedInt;
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
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header MyHeader1 {
    bit<8>          x1;
    int<8>          x2;
    EthernetAddress x3;
    MySignedInt     x4;
    varbit<8>       x5;
}

