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

extern Checksum16 {
    void clear();
    void update<D>(in D dt);
    void update<D>(in bool condition, in D dt);
    bit<16> get();
}

typedef bit<4> PortId_t;
const PortId_t REAL_PORT_COUNT = 4w8;
struct InControl {
    PortId_t inputPort;
}

const PortId_t RECIRCULATE_INPUT_PORT = 4w0xd;
const PortId_t CPU_INPUT_PORT = 4w0xe;
struct OutControl {
    PortId_t outputPort;
}

const PortId_t DROP_PORT = 4w0xf;
const PortId_t CPU_OUT_PORT = 4w0xe;
const PortId_t RECIRCULATE_OUT_PORT = 4w0xd;
parser Parser<H>(packet_in b, out H parsedHeaders);
control MAP<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package Simple<H>(Parser<H> p, MAP<H> map, Deparser<H> d);
struct Parsed_packet {
}

typedef bit<32> IPv4Address;
extern tbl {
}

control Pipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action(out PortId_t port) {
        port = 4w0xf;
    }
    action drop() {
    }
    table IPv4_match(in IPv4Address a) {
        actions = {
            drop;
        }
        key = {
            inCtrl.inputPort: exact;
        }
        implementation = tbl();
        default_action = drop;
    }
    apply {
        if (parseError != NoError) {
            Drop_action(outCtrl.outputPort);
            return;
        }
        IPv4Address nextHop;
        if (!IPv4_match.apply(nextHop).hit) 
            return;
    }
}

