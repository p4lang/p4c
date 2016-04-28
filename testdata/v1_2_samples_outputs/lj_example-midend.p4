#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "../testdata/v1_2_samples/simple_model.p4"

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
    @name("Drop_action") action Drop_action_0(out PortId_t port) {
        port = 4w0xf;
    }
    @name("Drop_1") action Drop() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Forward") action Forward_0(PortId_t outPort) {
        outCtrl.outputPort = outPort;
    }
    @name("Enet_lkup") table Enet_lkup_0() {
        key = {
            p.arpa_pak.dest: exact;
        }
        actions = {
            Drop_action_0(outCtrl.outputPort);
            Drop;
            Forward_0;
        }
        default_action = Drop;
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

Simple(LJparse(), LjPipe(), LJdeparse()) main;
