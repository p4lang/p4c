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

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header Mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

struct Pkthdr {
    Ethernet_h ethernet;
    Mpls_h[3]  mpls_vec;
}

parser X(packet_in b, out Pkthdr p) {
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
        }
    }
    state parse_mpls {
        b.extract<Mpls_h>(p.mpls_vec.next);
        transition select(p.mpls_vec.last.bos) {
            1w0: parse_mpls;
            1w1: parse_ipv4;
        }
    }
    state parse_ipv4 {
        transition accept;
    }
}

