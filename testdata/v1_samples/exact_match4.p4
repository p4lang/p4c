
header_type data1_t {
    fields {
        f1 : 128;
    }
}
header_type data2_t {
    fields {
        f2 : 128;
    }
}
header_type data3_t {
    fields {
        f3 : 128;
    }
}
header_type data_t {
    fields {
        f4 : 128;
        b1 : 8;
        b2 : 8;
    }
}
header data1_t data1;
header data2_t data2;
header data3_t data3;
header data_t data;

parser start {
    extract(data1);
    return d2;
}
parser d2 {
    extract(data2);
    return d3;
}
parser d3 {
    extract(data3);
    return more;
}
parser more {
    extract(data);
    return ingress;
}

action noop() { }
action setb1(val, port) {
    modify_field(data.b1, val);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
        data1.f1 : exact;
        data2.f2 : exact;
        data3.f3 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
