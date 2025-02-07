
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        b1 : 8;
        b2 : 8;
        b3 : 8;
        b4 : 8;
    }
}
header data_t data;
header_type data2_t {
    fields {
        x1 : 16;
        x2 : 16;
    }
}
header data2_t hdr1;
header data2_t hdr2;

parser start {
    extract(data);
    return select(data.b1) {
    0x00: parse_data2;
    default: ingress;
    }
}
parser parse_data2 {
    extract(hdr1);
    return select(hdr1.x1) {
    1 mask 1: parse_hdr2;
    default: ingress;
    }
}
parser parse_hdr2 {
    extract(hdr2);
    return ingress;
}

action noop() { }
action decap() {
    copy_header(hdr1, hdr2);
    remove_header(hdr2);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        decap;
        noop;
    }
}

control ingress {
    apply(test1);
}
