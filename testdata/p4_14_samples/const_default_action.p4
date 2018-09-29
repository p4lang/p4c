header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
        etherType : 16;
    }
}

/* METADATA */
header_type ingress_metadata_t {
    fields {
        ingress_port : 9;                         /* input physical port */
        bd : 14;
        rid : 14;
        drop_flag : 1;
    }
}

#define VLAN_DEPTH 2
header vlan_tag_t vlan_tag_[VLAN_DEPTH];
header ethernet_t ethernet;
metadata ingress_metadata_t ingress_metadata;

parser start {
    extract(ethernet);
    return select(latest.etherType) {
	default: parse_vlan;
    }
}

parser parse_vlan {
    extract(vlan_tag_[0]);
    return select(latest.etherType) {
        0x800 : ingress;
    }
}

action set_bd_properties(bd, ingress_rid) {
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.rid, ingress_rid);
}

action port_vlan_mapping_miss() {
    modify_field(ingress_metadata.drop_flag, 1);
}

action_profile bd_action_profile {
    actions {
        set_bd_properties;
        port_vlan_mapping_miss;
    }
    size : 1024;
}

action no_op(){}

table port_vlan_to_bd_mapping {
    reads {
        vlan_tag_[0] : valid;
        vlan_tag_[0].vid : ternary;
    }
    action_profile: bd_action_profile;
    const default_action: no_op();
    size : 1024;
}

table vlan_to_bd_mapping {
    reads {
        vlan_tag_[0].vid : exact;
    }
    action_profile: bd_action_profile;
    const default_action: no_op();
    size : 1024;
}

control process_port_vlan_mapping {
    apply(port_vlan_to_bd_mapping) {
	    miss { apply(vlan_to_bd_mapping); }
    }
}

control ingress {
    /* derive bd and its properties  */
    process_port_vlan_mapping();
}

control egress {}
