#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>

#define VLAN_DEPTH             2
#define ETHERTYPE_VLAN         0x8100
#define ETHERTYPE_IPV4         0x0800

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

header_type new_tag_24t {
    fields {
        t24_f1_6b : 6;
        t24_f2_12b : 12;
        t24_f3_14b : 14;
    }
}

header_type new_tag_32t {
    fields {
        t32_f1_16b : 16;
        t32_f2_16b : 16;
    }
}

header_type new_tag_48t {
    fields {
        t48_f1_16b : 16;
        t48_f2_32b : 32;
    }
}

header_type new_tag_64t {
    fields {
        t64_f1_48b : 48;
        t64_f2_16b : 16;
    }
}

header ethernet_t ethernet;
header vlan_tag_t vlan_tag_;
header new_tag_24t new_tag24_;
header new_tag_32t new_tag32_;
header new_tag_48t new_tag48_;
header new_tag_64t new_tag64_;
header new_tag_64t new_tag64_2_;

parser start {
    return parse_ethernet;
}

@pragma parser_value_set_size 5
parser_value_set pvs1;
@pragma parser_value_set_size 7
parser_value_set pvs2;
@pragma parser_value_set_size 9
parser_value_set pvs3;
@pragma parser_value_set_size 2
parser_value_set pvs4;
@pragma parser_value_set_size 5
parser_value_set pvs5;


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        pvs2 : parse_vlan;
        default : ingress;
    }
}

parser parse_vlan {
    extract(vlan_tag_);
    return ingress;
}


// Example of using PVS along with constant select value
parser parse_hdr_pvs1 {
    extract(new_tag24_);
    return select(latest.t24_f1_6b, latest.t24_f3_14b) {
      pvs3 : parse_hdr_pvs3;
      199: parse_hdr_pvs4;
      default: ingress;
    }
}

parser parse_hdr_pvs2 {
    extract(new_tag32_);
    return ingress;
}

parser parse_hdr_pvs3 {
    extract(new_tag48_);
    return select(latest.t48_f2_32b) {
      pvs4: parse_hdr_pvs4;
      default: ingress;
    }
}

parser parse_hdr_pvs4 {
    extract(new_tag64_);
    // Expected to get compile error when branch condition value is > 32bits
    //return select(latest.t64_f1_48b) {
    //  6700: parse_hdr_pvs5;
    //  default: ingress;
    //}
    return ingress;
}
    
parser parse_hdr_pvs5 {
    extract(new_tag64_2_);
    return ingress;
}


action vlan_miss(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action vlan_hit(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

table vlan {
    reads {
        vlan_tag_.vid: exact;
    }
    actions {
        vlan_miss;
        vlan_hit;
    }
    size : 512;
}


action noop() {
    no_op();
}


control ingress {
    apply(vlan);
}

