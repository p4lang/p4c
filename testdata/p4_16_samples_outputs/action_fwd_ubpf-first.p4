#include <core.p4>
#include <ubpf_model.p4>

struct Headers_t {
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    apply {
        if (std_meta.input_port == 32w1) {
            std_meta.output_port = 32w2;
        } else if (std_meta.input_port == 32w2) {
            std_meta.output_port = 32w1;
        } else {
            return;
        }
        std_meta.output_action = ubpf_action.REDIRECT;
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
