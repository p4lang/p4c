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

action do_xor() { bit_xor(data.b1, data.b2, data.b3); }
action do_and() { bit_and(data.b2, data.b3, data.b4); }
action do_or() { bit_or(data.b4, data.b3, data.b1); }
action do_add() { add(data.b3, data.b1, data.b2); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        do_add;
        do_and;
        do_or;
        do_xor;
    }
}

control ingress {
    apply(test1);
}
