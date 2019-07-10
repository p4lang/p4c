#include <core.p4>

typedef bit<4> PortId;
struct InControl {
    PortId inputPort;
}

struct OutControl {
    PortId outputPort;
}

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
        b.extract<ARPA_hdr>(p.arpa_pak);
        transition accept;
    }
}

control LjPipe(inout Parsed_rep p, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    @name("LjPipe.Drop_action") action Drop_action(out PortId port) {
        port = 4w0xf;
    }
    @name("LjPipe.Drop_1") action Drop_0() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("LjPipe.Forward") action Forward(PortId outPort) {
        outCtrl.outputPort = outPort;
    }
    @name("LjPipe.Enet_lkup") table Enet_lkup_0 {
        key = {
            p.arpa_pak.dest: exact @name("p.arpa_pak.dest") ;
        }
        actions = {
            Drop_action(outCtrl.outputPort);
            Drop_0();
            Forward();
        }
        default_action = Drop_0();
    }
    apply {
        outCtrl.outputPort = 4w0xf;
        if (p.arpa_pak.isValid()) {
            Enet_lkup_0.apply();
        }
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b) {
    apply {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
}

VSS<Parsed_rep>(LJparse(), LjPipe(), LJdeparse()) main;

