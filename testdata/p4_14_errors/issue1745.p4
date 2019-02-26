header_type hd_t {
    fields {
        options        : *;
        length_        : 8;
    }
    length: length_;
    max_length: 60;
}

header hd_t     hd;

parser start {
    extract(hd);
    return ingress;
}

control ingress { }
