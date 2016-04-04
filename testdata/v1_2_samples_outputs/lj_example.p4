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
    action Drop_action(out PortId_t port) {
        port = DROP_PORT;
    }
    action Drop_1() {
        outCtrl.outputPort = DROP_PORT;
    }
    action Forward(PortId_t outPort) {
        outCtrl.outputPort = outPort;
    }
    table Enet_lkup() {
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
        if (p.arpa_pak.valid) 
            Enet_lkup.apply();
    }
}

control LJdeparse(inout Parsed_rep p, packet_out b) {
    apply {
        b.emit<ARPA_hdr>(p.arpa_pak);
    }
}

Simple(LJparse(), LjPipe(), LJdeparse()) main;
