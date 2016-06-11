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

extern Checksum16 {
    void clear();
    void update<D>(in D dt);
    void update<D>(in bool condition, in D dt);
    bit<16> get();
}

typedef bit<4> PortId_t;
struct InControl {
    PortId_t inputPort;
}

struct OutControl {
    PortId_t outputPort;
}

