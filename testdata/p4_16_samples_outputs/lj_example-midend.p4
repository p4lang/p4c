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

parser Parser<H>(packet_in b, out H parsedHeaders);
control MAP<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package Simple<H>(Parser<H> p, MAP<H> map, Deparser<H> d);
header ARPA_hdr {
    bit<48> src;
    bit<48> dest;
    bit<16> etype;
}

struct Parsed_rep {
    ARPA_hdr arpa_pak;
}

parser LJparse(packet_in b, out Parsed_rep p) {
    state start {
        b.extract<ARPA_hdr>(p.arpa_pak);
        transition accept;
    }
}

control LjPipe(inout Parsed_rep p, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    PortId_t port_0;
    @name("Drop_action") action Drop_action() {
        port_0 = 4w0xf;
        outCtrl.outputPort = port_0;
    }
    @name("Drop_1") action Drop_0() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Forward") action Forward(PortId_t outPort) {
        outCtrl.outputPort = outPort;
    }
    @name("Enet_lkup") table Enet_lkup_0() {
        key = {
            p.arpa_pak.dest: exact;
        }
        actions = {
            Drop_action();
            Drop_0();
            Forward();
        }
        default_action = Drop_0();
    }
    apply {
        if (p.arpa_pak.isValid()) 
            Enet_lkup_0.apply();
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b) {
    apply {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
}

Simple<Parsed_rep>(LJparse(), LjPipe(), LJdeparse()) main;
