header_type mpls_t {
    fields {
        label : 20;
        tc : 3;
        bos : 1;
        ttl : 8;
    }
}

header mpls_t value[5];

parser start {
    extract(value[next]);
    return select(latest.bos) {
        0: start;
        1: ingress;
    }
}

table table_clean_bos{
    actions{ clean_bos; }
    size: 1;
}

action clean_bos() {
    modify_field(value[last].bos, 0);
}

control ingress {
    apply(table_clean_bos);
}