#include <core.p4>
#include <v1model.p4>

header cfi_t {
    bit<1> cfi;
}

header len_or_type_t {
    bit<16> value;
}

header mac_da_t {
    bit<48> mac;
}

header mac_sa_t {
    bit<48> mac;
}

header pcp_t {
    bit<3> pcp;
}

header vlan_id_t {
    bit<12> vlan_id_t;
}

struct metadata {
}

struct headers {
    @name(".cfi") 
    cfi_t         cfi;
    @name(".len_or_type") 
    len_or_type_t len_or_type;
    @name(".mac_da") 
    mac_da_t      mac_da;
    @name(".mac_sa") 
    mac_sa_t      mac_sa;
    @name(".pcp") 
    pcp_t         pcp;
    @name(".vlan_id") 
    vlan_id_t     vlan_id;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_cfi") state parse_cfi {
        packet.extract<cfi_t>(hdr.cfi);
        transition parse_vlan_id;
    }
    @name(".parse_len_or_type") state parse_len_or_type {
        packet.extract<len_or_type_t>(hdr.len_or_type);
        transition parse_pcp;
    }
    @name(".parse_mac_da") state parse_mac_da {
        packet.extract<mac_da_t>(hdr.mac_da);
        transition parse_mac_sa;
    }
    @name(".parse_mac_sa") state parse_mac_sa {
        packet.extract<mac_sa_t>(hdr.mac_sa);
        transition parse_len_or_type;
    }
    @name(".parse_pcp") state parse_pcp {
        packet.extract<pcp_t>(hdr.pcp);
        transition parse_cfi;
    }
    @name(".parse_vlan_id") state parse_vlan_id {
        packet.extract<vlan_id_t>(hdr.vlan_id);
        transition accept;
    }
    @name(".start") state start {
        transition parse_mac_da;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".t2") table t2 {
        actions = {
            nop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.mac_sa.mac: exact @name("mac_sa.mac") ;
        }
        default_action = NoAction_0();
    }
    apply {
        t2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".nop") action nop_1() {
    }
    @name(".t1") table t1 {
        actions = {
            nop_1();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.mac_da.mac       : exact @name("mac_da.mac") ;
            hdr.len_or_type.value: exact @name("len_or_type.value") ;
        }
        default_action = NoAction_1();
    }
    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<mac_da_t>(hdr.mac_da);
        packet.emit<mac_sa_t>(hdr.mac_sa);
        packet.emit<len_or_type_t>(hdr.len_or_type);
        packet.emit<pcp_t>(hdr.pcp);
        packet.emit<cfi_t>(hdr.cfi);
        packet.emit<vlan_id_t>(hdr.vlan_id);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

