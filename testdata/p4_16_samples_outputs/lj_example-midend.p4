#include <core.p4>

struct InControl {
    bit<4> inputPort;
}

struct OutControl {
    bit<4> outputPort;
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
    @name("LjPipe.Drop_action") action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("LjPipe.Drop_1") action Drop_0() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("LjPipe.Forward") action Forward(@name("outPort") bit<4> outPort) {
        outCtrl.outputPort = outPort;
    }
    @name("LjPipe.Enet_lkup") table Enet_lkup_0 {
        key = {
            p.arpa_pak.dest: exact @name("p.arpa_pak.dest");
        }
        actions = {
            Drop_action();
            Drop_0();
            Forward();
        }
        default_action = Drop_0();
    }
    @hidden action lj_example70() {
        outCtrl.outputPort = 4w0xf;
    }
    @hidden table tbl_lj_example70 {
        actions = {
            lj_example70();
        }
        const default_action = lj_example70();
    }
    apply {
        tbl_lj_example70.apply();
        if (p.arpa_pak.isValid()) {
            Enet_lkup_0.apply();
        }
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b) {
    @hidden action lj_example79() {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
    @hidden table tbl_lj_example79 {
        actions = {
            lj_example79();
        }
        const default_action = lj_example79();
    }
    apply {
        tbl_lj_example79.apply();
    }
}

VSS<Parsed_rep>(LJparse(), LjPipe(), LJdeparse()) main;
