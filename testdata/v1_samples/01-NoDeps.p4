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

action set_egress_port(egress_port) {
    modify_field(ing_metadata.egress_port, egress_port);
}

table dmac {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

table smac_filter {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
        ing_drop;
    }
}

control ingress {
    apply(dmac);
    apply(smac_filter);
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
