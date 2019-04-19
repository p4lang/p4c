header_type data_t { fields { bit<32> x; } }
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action my_drop() {
    drop();
}

action set_egress_port(egress_port) {
    modify_field(standard_metadata.egress_spec, egress_port);
}

table repeater {
    reads {
        standard_metadata.ingress_port: exact;
    }
    actions {
        my_drop;
        set_egress_port;
    }
}

control ingress {
    apply(repeater);
}

control egress { }
