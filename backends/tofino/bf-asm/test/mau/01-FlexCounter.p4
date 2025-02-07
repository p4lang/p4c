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

header_type vlan_tag_t {
    fields {
        prio    : 3;
        cfi     : 1;
        vlan_id : 12;
    }
}

header ethernet_t ethernet;
header vlan_tag_t vlan_tag;

parser start {
    extract(ethernet);
    return select(latest.ethertype) {
        0x8100 : parse_vlan_tag;
        default: ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return ingress;
}

@pragma pa_solitary md.flex_counter_offset

header_type metadata_t {
    fields {
        flex_counter_index  : 13;
    }
}

metadata metadata_t md;

counter flex_counter {
    type : packets;
    instance_count : 8192;
}

action set_flex_counter_index(flex_counter_base) {
    add(md.flex_counter_index, flex_counter_base, vlan_tag.prio);    
}

action update_flex_counter() {
    count(flex_counter, md.flex_counter_index);
}

table vlan {
    reads {
        vlan_tag.vlan_id : exact;
    }
    actions {
        set_flex_counter_index;
    }
    size : 4096;
}

table update_counters {
    actions {
        update_flex_counter;
    }
}

control ingress {
    apply(vlan) {
        hit {
            apply(update_counters);
        }
    }        
}

control egress {
}
