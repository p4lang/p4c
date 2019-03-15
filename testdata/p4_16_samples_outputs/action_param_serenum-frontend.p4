#include <core.p4>

enum bit<16> EthTypes {
    IPv4 = 16w0x800,
    ARP = 16w0x806,
    RARP = 16w0x8035,
    EtherTalk = 16w0x809b,
    VLAN = 16w0x8100,
    IPX = 16w0x8137,
    IPv6 = 16w0x86dd
}

struct standard_metadata_t {
    EthTypes instance_type;
}

control c(inout standard_metadata_t sm, in EthTypes eth) {
    @name("c.a") action a(in EthTypes type) {
        sm.instance_type = type;
    }
    @name("c.t") table t_0 {
        actions = {
            a(eth);
        }
        default_action = a(eth);
    }
    apply {
        t_0.apply();
    }
}

control proto(inout standard_metadata_t sm, in EthTypes eth);
package top(proto p);
top(c()) main;

