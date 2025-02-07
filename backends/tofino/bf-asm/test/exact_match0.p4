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

action noop() { }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        noop;
    }
}

control ingress {
    apply(test1);
}
