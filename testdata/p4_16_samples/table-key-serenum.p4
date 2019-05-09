#include <v1model.p4>

enum bit<16> EthTypes {
    IPv4 = 0x0800,
    ARP = 0x0806,
    RARP = 0x8035,
    EtherTalk = 0x809B,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86DD
}

header Ethernet {
    bit<48> src;
    bit<48> dest;
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

control c(inout Headers h, inout standard_metadata_t sm) {
    action do_act(bit<32> type) {
	sm.instance_type = type;
    }
    table tns {
        key = {
            h.eth.type : exact;
        }
	actions = {
            do_act;
        }
	const entries = {
            EthTypes.IPv4 : do_act(0x0800);
	    EthTypes.VLAN : do_act(0x8100);
	}
    }

    apply {
        tns.apply();
    }

}

parser p<H>(packet_in _p, out H h);
control ctr<H, SM>(inout H h, inout SM sm);
package top<H, SM>(p<H> _p, ctr<H, SM> _c);

top(prs(), c()) main;
