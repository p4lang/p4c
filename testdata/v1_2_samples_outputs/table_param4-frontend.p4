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
    OverwritingHeader
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> sizeInBits);
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
    action b() {
    }
    table t(inout bit<32> x) {
        key = {
            x: exact;
        }
        actions = {
            a;
            b;
        }
        default_action = a;
    }
    apply {
        switch (t.apply(arg).action_run) {
            a: {
                t.apply(arg);
            }
            b: {
                arg = arg + 32w1;
            }
        }

    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
