struct Version {
    bit<8> major;
    bit<8> minor;
}

error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader,
    HeaderTooShort
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

control c(out bool x) {
    table t1() {
        key = {
            x: exact;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    table t2() {
        key = {
            x: exact;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        x = true;
        if (t1.apply().hit && t2.apply().hit) 
            x = false;
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
