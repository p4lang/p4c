#include <core.p4>

typedef bit<16> Base_t;
typedef Base_t Base1_t;
typedef Base1_t Base2_t;
typedef Base2_t EthT;
enum EthT EthTypes {
    IPv4 = 16w0x800,
    ARP = 16w0x806,
    RARP = 16w0x8035,
    EtherTalk = 16w0x809b,
    VLAN = 16w0x8100,
    IPX = 16w0x8137,
    IPv6 = 16w0x86dd
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
    @name("prs.e") Ethernet e_0;
    state start {
        e_0.setInvalid();
        p.extract<Ethernet>(e_0);
        transition select(e_0.type) {
            EthTypes.IPv4: accept;
            EthTypes.ARP: accept;
            default: reject;
        }
    }
}

control c(inout Headers h) {
    @name("c.hasReturned") bool hasReturned;
    apply {
        hasReturned = false;
        if (h.eth.isValid()) {
            ;
        } else {
            hasReturned = true;
        }
        if (hasReturned) {
            ;
        } else if (h.eth.type == EthTypes.IPv4) {
            h.eth.setInvalid();
        } else {
            h.eth.type = (EthTypes)16w0;
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;
