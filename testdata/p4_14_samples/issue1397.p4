
#define HDR1_SIZE 2
header_type hdr1_t {
    fields {
        totalLen: 16;
    }
}
header hdr1_t hdr1;

#define HDR2_SIZE 1
header_type hdr2_t {
    fields {
        f1: 8;
    }
}
header hdr2_t hdr2;

header_type byte_hdr_t {
    fields {
        f1: 8;
    }
}
header byte_hdr_t byte_hdr[3];

parser start {
    extract(hdr1);
    extract(hdr2);
    return select(hdr1.totalLen) {
        HDR1_SIZE + HDR2_SIZE     : ingress;
        HDR1_SIZE + HDR2_SIZE + 1 : parse_1bytes;
        HDR1_SIZE + HDR2_SIZE + 2 : parse_2bytes;
        HDR1_SIZE + HDR2_SIZE + 3 : parse_3bytes;
    }
}

parser parse_1bytes {
    extract(byte_hdr[0]);
    return ingress;
}
parser parse_2bytes {
    extract(byte_hdr[1]);
    return ingress;
}
parser parse_3bytes {
    extract(byte_hdr[2]);
    return ingress;
}

control ingress {
}

control egress {
}
