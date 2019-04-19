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
    bit<9>  _ingress_metadata_ingress_port0;
    bit<14> _ingress_metadata_bd1;
    bit<14> _ingress_metadata_rid2;
    bit<1>  _ingress_metadata_drop_flag3;
}

struct headers {
    @name(".ethernet") 
    ethernet_t    ethernet;
    @name(".vlan_tag_") 
    vlan_tag_t[2] vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_tag_t>(hdr.vlan_tag_[0]);
        transition select(hdr.vlan_tag_[0].etherType) {
            16w0x800: accept;
            default: noMatch;
        }
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            default: parse_vlan;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

@name(".bd_action_profile") action_profile(32w1024) bd_action_profile;

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bool _process_port_vlan_mapping_tmp;
    @name(".no_op") action _no_op_0() {
    }
    @name(".no_op") action _no_op_2() {
    }
    @name(".set_bd_properties") action _set_bd_properties_0(bit<14> bd, bit<14> ingress_rid) {
        meta._ingress_metadata_bd1 = bd;
        meta._ingress_metadata_rid2 = ingress_rid;
    }
    @name(".set_bd_properties") action _set_bd_properties_2(bit<14> bd, bit<14> ingress_rid) {
        meta._ingress_metadata_bd1 = bd;
        meta._ingress_metadata_rid2 = ingress_rid;
    }
    @name(".port_vlan_mapping_miss") action _port_vlan_mapping_miss_0() {
        meta._ingress_metadata_drop_flag3 = 1w1;
    }
    @name(".port_vlan_mapping_miss") action _port_vlan_mapping_miss_2() {
        meta._ingress_metadata_drop_flag3 = 1w1;
    }
    @name(".port_vlan_to_bd_mapping") table _port_vlan_to_bd_mapping {
        actions = {
            _set_bd_properties_0();
            _port_vlan_mapping_miss_0();
            @defaultonly _no_op_0();
        }
        key = {
            hdr.vlan_tag_[0].isValid(): exact @name("vlan_tag_[0].$valid$") ;
            hdr.vlan_tag_[0].vid      : ternary @name("vlan_tag_[0].vid") ;
        }
        size = 1024;
        const default_action = _no_op_0();
        implementation = bd_action_profile;
    }
    @name(".vlan_to_bd_mapping") table _vlan_to_bd_mapping {
        actions = {
            _set_bd_properties_2();
            _port_vlan_mapping_miss_2();
            @defaultonly _no_op_2();
        }
        key = {
            hdr.vlan_tag_[0].vid: exact @name("vlan_tag_[0].vid") ;
        }
        size = 1024;
        const default_action = _no_op_2();
        implementation = bd_action_profile;
    }
    @hidden action act() {
        _process_port_vlan_mapping_tmp = true;
    }
    @hidden action act_0() {
        _process_port_vlan_mapping_tmp = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (_port_vlan_to_bd_mapping.apply().hit) 
            tbl_act.apply();
        else 
            tbl_act_0.apply();
        if (_process_port_vlan_mapping_tmp) 
            ;
        else 
            _vlan_to_bd_mapping.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag_[0]);
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

