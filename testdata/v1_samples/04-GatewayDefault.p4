header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop : 8;
        egress_port : 8;
        f1 : 8;
        f2 : 16;
        f3 : 32;
        f4 : 64;     
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
ING_METADATA_ACTION(f1)
ING_METADATA_ACTION(f2)
ING_METADATA_ACTION(f3)
ING_METADATA_ACTION(f4)

table i_t1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        ing_drop;
        set_f1;
        set_f2;
        set_f3;
        set_egress_port;
    }
}

table i_t2 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_f2;
    }
}

table i_t3 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_f3;
    }
}

table i_t4 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_f4;
    }
}

control ingress {
    apply(i_t1) {
        nop {
            apply(i_t2);
            }
        set_egress_port {
            apply(i_t3);
            }
        default {
            apply(i_t4);
            }
    }
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
