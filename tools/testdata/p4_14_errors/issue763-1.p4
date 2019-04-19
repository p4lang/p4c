header_type X {
    fields {
        x: 32;
    }
}

header X standard_metadata;

parser start {
    return ingress;
}

control ingress {}
