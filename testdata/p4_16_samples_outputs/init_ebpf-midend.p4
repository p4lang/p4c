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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.match") action match(@name("act") bool act) {
        pass = act;
    }
    @name("pipe.tbl") table tbl_0 {
        key = {
            headers.ethernet.protocol: exact @name("headers.ethernet.protocol");
        }
        actions = {
            match();
            NoAction_1();
        }
        const entries = {
                        16w0x800 : match(true);
                        16w0xd000 : match(false);
        }
        implementation = hash_table(32w64);
        default_action = NoAction_1();
    }
    @hidden action init_ebpf58() {
        pass = true;
    }
    @hidden table tbl_init_ebpf58 {
        actions = {
            init_ebpf58();
        }
        const default_action = init_ebpf58();
    }
    apply {
        tbl_init_ebpf58.apply();
        tbl_0.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
