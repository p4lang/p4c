#include <core.p4>

extern Checksum16 {
    void clear();
    void update<D>(in D dt);
    void update<D>(in bool condition, in D dt);
    bit<16> get();
}

typedef bit<4> PortId_t;
const PortId_t REAL_PORT_COUNT = (PortId_t)4w8;
struct InControl {
    PortId_t inputPort;
}

const PortId_t RECIRCULATE_INPUT_PORT = (PortId_t)4w0xd;
const PortId_t CPU_INPUT_PORT = (PortId_t)4w0xe;
struct OutControl {
    PortId_t outputPort;
}

const PortId_t DROP_PORT = (PortId_t)4w0xf;
const PortId_t CPU_OUT_PORT = (PortId_t)4w0xe;
const PortId_t RECIRCULATE_OUT_PORT = (PortId_t)4w0xd;
parser Parser<H>(packet_in b, out H parsedHeaders);
control MAP<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package Simple<H>(Parser<H> p, MAP<H> map, Deparser<H> d);
