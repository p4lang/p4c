struct Version {
    bit<8> major;
    bit<8> minor;
}

const Version P4_VERSION = { 8w1, 8w2 };
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

extern void verify(in bool check, in error toSignal);
action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    action a() {
    }
    action b() {
    }
    table t(in bit<32> r) {
        key = {
            r: ternary;
        }
        actions = {
            a();
            b();
        }
        default_action = a();
    }
    apply {
        switch (t.apply(f(32w2)).action_run) {
            a: {
                if (f(32w2) < 32w2) 
                    r = 32w1;
                else 
                    r = 32w3;
            }
        }

    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
