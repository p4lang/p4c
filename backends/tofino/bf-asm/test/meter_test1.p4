header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        h3 : 16;
        h4 : 16;
        h5 : 16;
        h6 : 16;
        color_1 : 8;
        color_2 : 8;
    }
}

header data_t data;
parser start {
    extract (data);
    return ingress;
}

action h1_3(val1, val2, val3) {
    modify_field(data.h1, val1);
    modify_field(data.h2, val2);
    modify_field(data.h3, val3);
}

action h4_6(val4, val5, val6, port) {
    modify_field(data.h4, val4);
    modify_field(data.h5, val5);
    modify_field(data.h6, val6);
    modify_field(standard_metadata.egress_spec, port);
}


meter meter_1 {
    type : bytes;
    direct : test1;
    result : data.color_1;
}

meter meter_2 {
    type : bytes;
    direct : test2;
    result : data.color_2;
}

table test1 {
    reads {
        data.f1 : exact;
    } actions {
        h1_3;
    }
    size : 6000;
}

table test2 {
    reads {
        data.f2 : exact;
    } actions {
        h4_6;
    }
    size : 10000;
}

control ingress {
    apply(test1);
    apply(test2);
}
