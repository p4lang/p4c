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

action countb2(val) { count(simple, data.b2); }

counter simple {
    type : packets;
    instance_count : 256;
}

table test1 {
    actions {
        countb2;
    }
}

control ingress {
    apply(test1);
}
