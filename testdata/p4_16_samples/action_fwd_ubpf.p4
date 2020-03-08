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

    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;