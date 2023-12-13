#include <core.p4>

type bit<16> Base_t;
typedef Base_t Base1_t;
typedef Base1_t Base2_t;
typedef Base2_t EthT;
enum EthT EthTypes {
    IPv4 = 0x800,
    ARP = 0x806,
    RARP = 0x8035,
    EtherTalk = 0x809b,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86dd
}

typedef int<8> I8;
enum I8 UnrepresentableInt {
    representable_p = 127,
    representable_n = -128,
    unrepresentable_p = 128,
    unrepresentable_n = -129,
    explicit_cast = (I8)4 * 4 * 4 * 4
}

enum bit<4> UnrepresentableBit {
    unrepresentable_n = -1,
    representable_p = 15,
    unrepresentable_p = 16
}

header Ethernet {
    bit<48>  src;
    bit<48>  dest;
    EthTypes type;
}

struct Headers {
    Ethernet eth;
}

parser prs(packet_in p, out Headers h) {
    Ethernet e;
    state start {
        p.extract(e);
        transition select(e.type) {
            EthTypes.IPv4: accept;
            EthTypes.ARP: accept;
            default: reject;
        }
    }
}

control c(inout Headers h) {
    apply {
        if (!h.eth.isValid()) {
            return;
        }
        if (h.eth.type == EthTypes.IPv4) {
            h.eth.setInvalid();
        } else {
            h.eth.type = (EthTypes)(bit<16>)0;
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top(prs(), c()) main;
