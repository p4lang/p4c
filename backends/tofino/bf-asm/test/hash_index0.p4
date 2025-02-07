#include <tofino/intrinsic_metadata.p4>

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

parser start {
    extract(data);
    return ingress;
}

action setf1(val) { modify_field(data.f1, val); }

table test1 {
    reads {
        data.b1 : exact;
    }
    actions {
        setf1;
    }
    size : 256;
}

control ingress {
    apply(test1);
}
