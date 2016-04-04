header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp       : 3;
        cfi       : 1;
        vlan_id   : 12;
        ethertype : 16;
    }
}

header ethernet_t ethernet;
header vlan_tag_t vlan_tag;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag;
        default: ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return ingress; 
}

action nop() {
}

table t1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
    }
}

table t2 {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
    }
}

control ingress {
    apply(t1);
}

control egress {
    apply(t2);
}
