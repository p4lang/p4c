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

enum myenum1 {
    MY_ENUM1_VAL1,
    MY_ENUM1_VAL2,
    MY_ENUM1_VAL3
}

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

typedef tuple<bit<8>, bit<16>> myTuple0;
typedef tuple<bit<7>, int<33>, EthernetAddress, MySignedInt, varbit<56>, varbit<104>, error, bool, myenum1, Ethernet_h, Ethernet_h[4], mystruct1, mystruct2, myTuple0> myTuple1;
header MyHeader1 {
    bit<8>          x1;
    int<8>          x2;
    EthernetAddress x3;
    MySignedInt     x4;
    varbit<8>       x5;
}

