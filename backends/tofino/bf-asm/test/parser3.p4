
header_type data1_t {
    fields {
        f1 : 32;
        x1 : 2;
        x2 : 4;
        x3 : 4;
        x4 : 4;
        x5 : 2;
        x6 : 5;
        x7 : 2;
        x8 : 1;
    }
}
header data1_t data1;
header_type data2_t {
    fields {
        a1 : 8;
        a2 : 4;
        a3 : 4;
        a4 : 8;
        a5 : 4;
        a6 : 4;
    }
}
header data2_t data2;
#if 0
header_type data3_t {
    fields {
        b1 : 13;
        b2 : 3;
    }
}
header data3_t data3;
#endif

parser start {
    extract(data1);
    return select(data1.x3, data1.x1, data1.x7) {
        0xe4: parse_data2;
        default: ingress;
    }
}

parser parse_data2 {
    extract(data2);
    return ingress;
}

action noop() { }

table test1 {
    reads {
        data1.f1 : exact;
    }
    actions {
        noop;
    }
}

control ingress {
    apply(test1);
}
