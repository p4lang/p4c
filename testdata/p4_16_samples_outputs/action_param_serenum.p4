#include <core.p4>

enum bit<16> EthTypes {
    IPv4 = 0x800,
    ARP = 0x806,
    RARP = 0x8035,
    EtherTalk = 0x809b,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86dd
}

struct standard_metadata_t {
    EthTypes instance_type;
}

control c(inout standard_metadata_t sm, in EthTypes eth) {
    action a(in EthTypes type) {
        sm.instance_type = type;
    }
    table t {
        actions = {
            a(eth);
        }
        default_action = a(eth);
    }
    apply {
        t.apply();
    }
}

control proto(inout standard_metadata_t sm, in EthTypes eth);
package top(proto p);
top(c()) main;

