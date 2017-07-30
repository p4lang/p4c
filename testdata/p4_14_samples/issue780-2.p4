header_type main {
    fields {
        x : 32;
    }
}

header_type packet {
    fields {
        y: 32;
    }
}

header main heartlands;
header packet z;

parser start {
    return ingress;
}

action add_heartlands() {
    add_header(heartlands);
    add_header(z);
}

control ingress { }
