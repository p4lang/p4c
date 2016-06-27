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

extern void verify(in bool check, in error toSignal);
action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

struct EthernetHeader {
    bit<16> etherType;
}

struct IPv4 {
    bit<16> protocol;
}

struct Packet_header {
    EthernetHeader ethernet;
    IPv4           ipv4;
}

parser EthernetParser(packet_in b, out EthernetHeader h) {
    state start {
    }
}

parser GenericParser(packet_in b, out Packet_header p)(bool udpSupport) {
    EthernetParser() ethParser;
    state start {
        ethParser.apply(b, p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: ipv4;
        }
    }
    state ipv4 {
        b.extract<IPv4>(p.ipv4);
        transition select(p.ipv4.protocol) {
            16w6: tryudp;
            16w17: tcp;
        }
    }
    state tryudp {
        transition select(udpSupport) {
            false: reject;
            true: udp;
        }
    }
    state udp {
    }
    state tcp {
    }
}

