header_type metadata_t {
    fields {
        field_1_1_1  : 1;
        field_2_1_1  : 1;
    }
}

metadata metadata_t md;

parser start {
    return ingress;
}

#define ACTION(w, n)                                    \
action action_##w##_##n(value) {                        \
    modify_field(md.field_1_##w##_##n, value);          \
    modify_field(md.field_2_##w##_##n, w##n);           \
}

ACTION(1, 1)

table dmac {
    reads {
    }
    actions {
        action_1_1;
    }
}

control ingress {
    apply(dmac);
}

control egress {
}
