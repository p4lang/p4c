#include <core.p4>
#include <v1model.p4>

enum bit<16> EthTypes {
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
    Ethernet e;
    state start {
        p.extract<Ethernet>(e);
        transition select(e.type) {
            EthTypes.IPv4: accept;
            EthTypes.ARP: accept;
            default: reject;
        }
    }
}

control c(inout Headers h, inout standard_metadata_t sm) {
    action do_act(bit<32> type) {
        sm.instance_type = type;
    }
    table tns {
        key = {
            h.eth.type: exact @name("h.eth.type") ;
        }
        actions = {
            do_act();
            @defaultonly NoAction();
        }
        const entries = {
                        EthTypes.IPv4 : do_act(32w0x800);

                        EthTypes.VLAN : do_act(32w0x8100);

        }

        default_action = NoAction();
    }
    apply {
        tns.apply();
    }
}

parser p<H>(packet_in _p, out H h);
control ctr<H, SM>(inout H h, inout SM sm);
package top<H, SM>(p<H> _p, ctr<H, SM> _c);
top<Headers, standard_metadata_t>(prs(), c()) main;

