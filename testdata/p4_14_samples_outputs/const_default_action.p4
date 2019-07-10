#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<9>  ingress_port;
    bit<14> bd;
    bit<14> rid;
    bit<1>  drop_flag;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
    @name(".ingress_metadata") 
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t    ethernet;
    @name(".vlan_tag_") 
    vlan_tag_t[2] vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_vlan") state parse_vlan {
        packet.extract(hdr.vlan_tag_[0]);
        transition select(hdr.vlan_tag_[0].etherType) {
            16w0x800: accept;
        }
    }
    @name(".start") state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            default: parse_vlan;
        }
    }
}

@name(".bd_action_profile") action_profile(32w1024) bd_action_profile;

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control process_port_vlan_mapping(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".no_op") action no_op() {
    }
    @name(".set_bd_properties") action set_bd_properties(bit<14> bd, bit<14> ingress_rid) {
        meta.ingress_metadata.bd = bd;
        meta.ingress_metadata.rid = ingress_rid;
    }
    @name(".port_vlan_mapping_miss") action port_vlan_mapping_miss() {
        meta.ingress_metadata.drop_flag = 1w1;
    }
    @name(".port_vlan_to_bd_mapping") table port_vlan_to_bd_mapping {
        actions = {
            set_bd_properties;
            port_vlan_mapping_miss;
            @defaultonly no_op;
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact;
            hdr.vlan_tag_[0].vid      : ternary;
        }
        size = 1024;
        const default_action = no_op();
        implementation = bd_action_profile;
    }
    @name(".vlan_to_bd_mapping") table vlan_to_bd_mapping {
        actions = {
            set_bd_properties;
            port_vlan_mapping_miss;
            @defaultonly no_op;
        }
        key = {
            hdr.vlan_tag_[0].vid: exact;
        }
        size = 1024;
        const default_action = no_op();
        implementation = bd_action_profile;
    }
    apply {
        if (port_vlan_to_bd_mapping.apply().hit) {
            ;
        } else {
            vlan_to_bd_mapping.apply();
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".process_port_vlan_mapping") process_port_vlan_mapping() process_port_vlan_mapping_0;
    apply {
        process_port_vlan_mapping_0.apply(hdr, meta, standard_metadata);
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag_[0]);
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

