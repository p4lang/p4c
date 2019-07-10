#include <core.p4>

typedef bit<4> PortId;
const PortId REAL_PORT_COUNT = 4w8;
struct InControl {
    PortId inputPort;
}

const PortId RECIRCULATE_IN_PORT = 0xd;
const PortId CPU_IN_PORT = 0xe;
struct OutControl {
    PortId outputPort;
}

const PortId DROP_PORT = 0xf;
const PortId CPU_OUT_PORT = 0xe;
const PortId RECIRCULATE_OUT_PORT = 0xd;
parser Parser<H>(packet_in b, out H parsedHeaders);
control Pipe<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package VSS<H>(Parser<H> p, Pipe<H> map, Deparser<H> d);
extern Ck16 {
    Ck16();
    void clear();
    void update<T>(in T data);
    bit<16> get();
}

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
        b.extract(p.arpa_pak);
        transition accept;
    }
}

control LjPipe(inout Parsed_rep p, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action(out PortId port) {
        port = DROP_PORT;
    }
    action Drop_1() {
        outCtrl.outputPort = DROP_PORT;
    }
    action Forward(PortId outPort) {
        outCtrl.outputPort = outPort;
    }
    table Enet_lkup {
        key = {
            p.arpa_pak.dest: exact;
        }
        actions = {
            Drop_action(outCtrl.outputPort);
            Drop_1;
            Forward;
        }
        default_action = Drop_1;
    }
    apply {
        outCtrl.outputPort = DROP_PORT;
        if (p.arpa_pak.isValid()) {
            Enet_lkup.apply();
        }
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b) {
    apply {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
}

VSS(LJparse(), LjPipe(), LJdeparse()) main;

