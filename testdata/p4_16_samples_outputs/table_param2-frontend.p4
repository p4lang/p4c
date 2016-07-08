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

match_kind {
    exact,
    ternary,
    lpm
}

control c(inout bit<32> arg) {
    action a() {
    }
    table t(inout bit<32> x) {
        key = {
            x: exact;
        }
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        if (t.apply(arg).hit) 
            arg = 32w0;
        else 
            arg = arg + 32w1;
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
