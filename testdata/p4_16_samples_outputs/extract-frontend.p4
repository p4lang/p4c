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

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    state start {
        p.extract<H>(h.next);
        p.extract<H>(h.next);
        p.extract<H>(h.next);
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<H[2]>(P()) main;
