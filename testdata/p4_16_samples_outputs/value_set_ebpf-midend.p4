#include <core.p4>
#include <ebpf_model.p4>

header test_header {
    bit<8> value;
}

header next_header {
    bit<8> value;
}

struct Headers_t {
    test_header first;
    next_header next;
}

parser prs(packet_in p, out Headers_t headers) {
    @name("prs.pvs") value_set<bit<8>>(4) pvs_0;
    state start {
        p.extract<test_header>(headers.first);
        transition select(headers.first.value) {
            pvs_0: parse_next;
            default: accept;
        }
    }
    state parse_next {
        p.extract<next_header>(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    @hidden action value_set_ebpf54() {
        pass = true;
    }
    @hidden action value_set_ebpf56() {
        pass = false;
    }
    @hidden table tbl_value_set_ebpf54 {
        actions = {
            value_set_ebpf54();
        }
        const default_action = value_set_ebpf54();
    }
    @hidden table tbl_value_set_ebpf56 {
        actions = {
            value_set_ebpf56();
        }
        const default_action = value_set_ebpf56();
    }
    apply {
        if (headers.next.isValid()) {
            tbl_value_set_ebpf54.apply();
        } else {
            tbl_value_set_ebpf56.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
