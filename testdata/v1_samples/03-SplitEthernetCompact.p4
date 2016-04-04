header_type mac_da_t {
    fields {
        mac   : 48;
    }
}

header_type mac_sa_t {
    fields {
        mac   : 48;
    }
}

header_type len_or_type_t {
    fields {
        value   : 48;
    }
}

header mac_da_t      mac_da;
header mac_sa_t      mac_sa;
header len_or_type_t len_or_type;

parser start {
    extract(mac_da);
    extract(mac_sa);
    extract(len_or_type);
    return ingress;
}

action nop() {
}

table t1 {
    reads {
        mac_da.mac : exact;
        len_or_type.value: exact;
    }
    actions {
        nop;
    }
}

table t2 {
    reads {
        mac_sa.mac : exact;
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
