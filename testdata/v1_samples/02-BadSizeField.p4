header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 15;
    }
}

header ethernet_t ethernet;

header_type vag_t {
    fields {
        f1 : 33;
        f2 : 7;
    }
}

header vag_t vag;

parser start {
    extract(ethernet);
    extract(vag);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop        : 1;
        egress_port : 8;
    }
} 

metadata ingress_metadata_t ing_metadata;

action nop() {
}

action ing_drop() {
    modify_field(ing_metadata.drop, 1);
}

#define ING_METADATA_ACTION(f)           \
action set_##f (f) {                     \
    modify_field(ing_metadata.f, f);   \
}

ING_METADATA_ACTION(egress_port)

table i_t1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

control ingress {
    apply(i_t1);
}

table e_t1 {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
    }
}

control egress {
    apply(e_t1);
}
