header_type standard_metadata_t {
    fields {
        x: 32;
    }
}

parser start {
    return ingress;
}

control ingress {}
