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

header B {
    varbit<120> field1;
}

parser P(packet_in p) {
    B b;
    state start {
        b.setInvalid();
        p.extract<B>(b, 32w10);
        transition start;
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
