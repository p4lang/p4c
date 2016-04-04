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

header_type my_tag_t {
    fields {
        bd : 32;
        ethertype : 16;
    }
}

#define N_TAGS 1

#if N_TAGS > 1 
#  define ARRAY_DECL [N_TAGS]
#  define ARRAY_REF  [next]
#else 
#  define ARRAY_DECL
#  define ARRAY_REF 
#endif

header ethernet_t ethernet;
header vlan_tag_t vlan_tag ARRAY_DECL;
header my_tag_t   my_tag   ARRAY_DECL;

parser start {
    extract(ethernet);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag_outer;
        0x9000            : parse_my_tag_outer;
        default: ingress;
    }
}

parser parse_vlan_tag_outer {
    extract(vlan_tag ARRAY_REF);
    return select(latest.ethertype) {
        0x9000            : parse_my_tag_inner;
        default: ingress;
    }
}

parser parse_my_tag_outer {
    extract(my_tag ARRAY_REF);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag_inner;
        default: ingress;
    }
}

parser parse_vlan_tag_inner {
    extract(vlan_tag ARRAY_REF);
    return select(latest.ethertype) {
        default: ingress;
    }
}

parser parse_my_tag_inner {
    extract(my_tag ARRAY_REF);
    return select(latest.ethertype) {
        default: ingress;
    }
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
