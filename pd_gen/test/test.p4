header_type header_test_t {
    fields {
        field8 : 8;
        field16 : 16;
        field20 : 20;
        field24 : 24;
        field32 : 32;
        field48 : 48;
        field64 : 64;
    }
}

header header_test_t header_test;
header header_test_t header_test_1;

parser start {
    return ingress;
}

action actionA(param) {
    modify_field(header_test.field48, param);
}

action actionB(param) {
    modify_field(header_test.field8, param);
}

table ExactOne {
    reads {
         header_test.field32 : exact;
    }
    actions {
        actionA; actionB;
    }
    size: 512;
}

counter ExactOne_counter {
    type : packets;
    direct : ExactOne;
}

table LpmOne {
    reads {
         header_test.field32 : lpm;
    }
    actions {
        actionA;
    }
    size: 512;
}

table TernaryOne {
    reads {
         header_test.field32 : ternary;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactOneNA {
    reads {
         header_test.field20 : exact;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactTwo {
    reads {
         header_test.field32 : exact;
         header_test.field16 : exact;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactAndValid {
    reads {
         header_test.field32 : exact;
         header_test_1 : valid;
    }
    actions {
        actionA;
    }
    size: 512;
}

#define LEARN_RECEIVER 1

field_list LearnDigest {
    header_test.field32;
    header_test.field16;
}

action ActionLearn() {
    generate_digest(LEARN_RECEIVER, LearnDigest);
}

table Learn {
    reads {
         header_test.field32 : exact;
    }
    actions {
        ActionLearn;
    }
    size: 512;
}

control ingress {
    apply(ExactOne);
    apply(LpmOne);
    apply(TernaryOne);
    apply(ExactOneNA);
    apply(ExactTwo);
    apply(ExactAndValid);
    apply(Learn);
}

control egress {

}


