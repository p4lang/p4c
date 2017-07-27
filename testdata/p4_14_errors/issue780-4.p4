header_type egress {
    fields {
        counterrevolution : 32;
    }
}
header egress heartlands;
parser start {
    return ingress;
}
action add_heartlands() {
    add_header(heartlands);
}
control ingress { }
