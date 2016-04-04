header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

header_type vag_t {
    fields {
        f1 : 1024;
        f2 : 512;
        f3 : 256;
        f4 : 128;
    }
}

header vag_t vag;

parser start {
    extract(ethernet);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop        : 1;
        egress_port : 8;
        f1 : 1024;
        f2 : 512;
        f3 : 256;
        f4 : 128;
        
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
        vag.f1 : exact;
    }
    actions {
        nop;
        set_f1;
    }
}

table i_t2 {
    reads {
        vag.f2 : exact;
    }
    actions {
        nop;
        set_f2;
    }
}

table i_t3 {
    reads {
        vag.f3 : exact;
    }
    actions {
        nop;
        set_f3;
    }
}

table i_t4 {
    reads {
        vag.f4 : ternary;
    }
    actions {
        nop;
        set_f4;
    }
}

control ingress {
    apply(i_t1);
    apply(i_t2);
    apply(i_t3);
    apply(i_t4);
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
