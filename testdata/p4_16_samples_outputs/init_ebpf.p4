#include <core.p4>
#include <ebpf_model.p4>

header Ethernet {
    bit<48> destination;
    bit<48> source;
    bit<16> protocol;
}

struct Headers_t {
    Ethernet ethernet;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    action match(bool act) {
        pass = act;
    }
    table tbl {
        key = {
            headers.ethernet.protocol: exact;
        }
        actions = {
            match;
            NoAction;
        }
        const entries = {
                        0x800 : match(true);

                        0xd000 : match(false);

        }

        implementation = hash_table(64);
    }
    apply {
        pass = true;
        tbl.apply();
    }
}

ebpfFilter(prs(), pipe()) main;

