#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "../testdata/v1_2_samples/simple_model.p4"

struct Parsed_packet {
}

typedef bit<32> IPv4Address;
extern tbl {
}

control Pipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action(out PortId_t port) {
        port = DROP_PORT;
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

