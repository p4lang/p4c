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
        packet.extract(hdr.cfi);
        transition parse_vlan_id;
    }
    @name(".parse_len_or_type") state parse_len_or_type {
        packet.extract(hdr.len_or_type);
        transition parse_pcp;
    }
    @name(".parse_mac_da") state parse_mac_da {
        packet.extract(hdr.mac_da);
        transition parse_mac_sa;
    }
    @name(".parse_mac_sa") state parse_mac_sa {
        packet.extract(hdr.mac_sa);
        transition parse_len_or_type;
    }
    @name(".parse_pcp") state parse_pcp {
        packet.extract(hdr.pcp);
        transition parse_cfi;
    }
    @name(".parse_vlan_id") state parse_vlan_id {
        packet.extract(hdr.vlan_id);
        transition accept;
    }
    @name(".start") state start {
        transition parse_mac_da;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t2") table t2 {
        actions = {
            nop;
        }
        key = {
            hdr.mac_sa.mac: exact;
        }
    }
    apply {
        t2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t1") table t1 {
        actions = {
            nop;
        }
        key = {
            hdr.mac_da.mac       : exact;
            hdr.len_or_type.value: exact;
        }
    }
    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.mac_da);
        packet.emit(hdr.mac_sa);
        packet.emit(hdr.len_or_type);
        packet.emit(hdr.pcp);
        packet.emit(hdr.cfi);
        packet.emit(hdr.vlan_id);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

