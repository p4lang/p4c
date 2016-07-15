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

parser Prs<T>(packet_in b, out T result);
control Map<T>(in T d);
package Switch<T>(Prs<T> prs, Map<T> map);
parser P(packet_in b, out bit<32> d) {
    state start {
    }
}

control Map1(in bit<32> d) {
    apply {
    }
}

Switch<bit<32>>(P(), Map1()) main;
