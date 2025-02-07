#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
#else
#include "includes/tofino.p4"
#endif

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t outer_ethernet;
header ethernet_t inner_ethernet;

header_type vlan_tag_t {
    fields {
        pri       : 3;
        cfi       : 1;
        vid       : 12;
        ethertype : 16;
    }
}

header vlan_tag_t vlan_tag;

header_type trill_t {
    fields {
        version : 2;
        reserved : 2;
        multiDestination : 1;
        optLength : 5;
        hopCount : 6;
        egressRbridge : 16;
        ingressRbridge : 16;
    }
}

header trill_t trill;

parser start {
    extract(outer_ethernet);
    return select(latest.ethertype) {
        0x8100 : parse_vlan_tag;
        0x2222 : parse_trill;
        default : ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.ethertype) {
        0x2222 : parse_trill;
        default : ingress;
    }
}

parser parse_trill {
    extract(trill);
    extract(inner_ethernet);
    return ingress;
}

/*
 * TRILL Forwarding Table
 */
action forward_trill(new_mac_da, new_mac_sa, new_vlan_id, new_port) {
    modify_field(outer_ethernet.dstAddr, new_mac_da);
    modify_field(outer_ethernet.srcAddr, new_mac_sa);
    modify_field(vlan_tag.vid, new_vlan_id);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, new_port);
    add_to_field(trill.hopCount, -1);
}

table trill_forward {
    reads {
        trill.egressRbridge : exact;
    }
    actions {
        forward_trill;
    }
}

control ingress {
    apply(trill_forward);
}


control egress {
}
