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
        p.extract<Ethernet>(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    action match(bool act) {
        pass = act;
    }
    table tbl {
        key = {
            headers.ethernet.protocol: exact @name("headers.ethernet.protocol") ;
        }
        actions = {
            match();
            NoAction();
        }
        const entries = {
                        16w0x800 : match(true);

                        16w0xd000 : match(false);

        }

        implementation = hash_table(32w64);
        default_action = NoAction();
    }
    apply {
        pass = true;
        tbl.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

