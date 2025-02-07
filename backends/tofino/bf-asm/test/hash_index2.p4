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
action setf2(val) { modify_field(data.f2, val); }
action setf3(val) { modify_field(data.f3, val); }

@pragma use_hash_action 1
table test1 {
    reads {
        data.b1 : exact;
    }
    actions {
        setf1;
    }
    default_action : setf1(0);
    size : 256;
}

table test2 {
    reads {
        data.f1 : ternary;
    }
    actions {
        setf2;
        setf3;
    }
}

control ingress {
    apply(test1);
    apply(test2);
}
