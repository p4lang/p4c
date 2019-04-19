#include <core.p4>

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
header H {
    error         x7;
    bool          x8;
    myenum1       x9;
    Ethernet_h    x10;
    Ethernet_h[4] x11;
    mystruct1     x12;
    mystruct2     x13;
    myTuple0      x14;
}

