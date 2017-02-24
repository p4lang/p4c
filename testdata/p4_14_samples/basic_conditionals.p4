header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        h3 : 16;
        h4 : 16;
    }
} 

header data_t data;

parser start {
    extract(data);
    return ingress;
}

action set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
       data.f1 : exact;
    }
    actions {
        set_port;
    }
}

control ingress {
    if (true) {
        apply(test1);
    }
}
