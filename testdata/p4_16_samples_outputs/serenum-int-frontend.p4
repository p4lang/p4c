#include <core.p4>

enum int<24> EthTypes {
    IPv4 = 24s0x800,
    ARP = 24s0x806,
    RARP = 24s0x8035,
    EtherTalk = 24s0x809b,
    VLAN = 24s0x8100,
    IPX = 24s0x8137,
    IPv6 = 24s0x86dd
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
            h.eth.type = (EthTypes)24s0;
        }
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H>(inout H h);
package top<H>(p<H> _p, ctr<H> _c);
top<Headers>(prs(), c()) main;
