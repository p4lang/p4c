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

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header h) {
    bit<1> x_0;
    state start {
        transition select(x_0) {
            1w0: chain1;
            1w1: chain2;
        }
    }
    state chain1 {
        p.extract<Header>(h);
        transition endchain;
    }
    state chain2 {
        p.extract<Header>(h);
        transition endchain;
    }
    state endchain {
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;
