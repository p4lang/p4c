#include <core.p4>


typedef bit<4> PortId;
const PortId REAL_PORT_COUNT = 4w8;
struct InControl {
    PortId inputPort;
}

const PortId RECIRCULATE_IN_PORT = 4w0xd;
const PortId CPU_IN_PORT = 4w0xe;
struct OutControl {
    PortId outputPort;
}

const PortId DROP_PORT = 4w0xf;
const PortId CPU_OUT_PORT = 4w0xe;
const PortId RECIRCULATE_OUT_PORT = 4w0xd;
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

struct Parsed_packet {
}

typedef bit<32> IPv4Address;
extern tbl {
    tbl();
}

control c(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action(out PortId port) {
        port = 4w0xf;
    }
    action drop() {
    }
    table IPv4_match(in IPv4Address a) {
        actions = {
            drop();
        }
        key = {
            inCtrl.inputPort: exact;
        }
        implementation = tbl();
        default_action = drop();
    }
    apply {
        if (parseError != error.NoError) {
            Drop_action(outCtrl.outputPort);
            return;
        }
        IPv4Address nextHop;
        if (!IPv4_match.apply(nextHop).hit) 
            return;
    }
}

