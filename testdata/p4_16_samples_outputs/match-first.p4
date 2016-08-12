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

header Ipv4_no_options_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header Ipv4_options_h {
    varbit<160> options;
}

struct Tcp {
    bit<16> port;
}

struct Parsed_headers {
    Ipv4_no_options_h ipv4;
    Ipv4_options_h    ipv4options;
    Tcp               tcp;
}

error {
    InvalidOptions
}

parser Top(packet_in b, out Parsed_headers headers) {
    state start {
        transition parse_ipv4;
    }
    state parse_ipv4 {
        b.extract<Ipv4_no_options_h>(headers.ipv4);
        verify(headers.ipv4.ihl >= 4w5, InvalidOptions);
        transition parse_ipv4_options;
    }
    state parse_ipv4_options {
        b.extract<Ipv4_options_h>(headers.ipv4options, (bit<32>)(((bit<8>)headers.ipv4.ihl << 2) + 8w236 << 3));
        transition select(headers.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        b.extract<Tcp>(headers.tcp);
        transition select(headers.tcp.port) {
            16w0 &&& 16w0xfc00: well_known_port;
            default: other_port;
        }
    }
    state well_known_port {
        transition accept;
    }
    state other_port {
        transition accept;
    }
    state parse_udp {
        transition accept;
    }
}

