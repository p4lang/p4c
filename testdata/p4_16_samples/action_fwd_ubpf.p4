#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
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
        if (std_meta.input_port == 1) {
            std_meta.output_port = 2;
        } else if (std_meta.input_port == 2) {
            std_meta.output_port = 1;
        } else {
            return;
        }
        std_meta.output_action = ubpf_action.REDIRECT;
    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;