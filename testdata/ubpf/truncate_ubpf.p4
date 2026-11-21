#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

header truncate_spec_t {
    bit<8>  bytes_to_save;
}
header next_header_t {
    bit<32> data;
}

struct Headers_t {
    truncate_spec_t spec;
    next_header_t   data;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.spec);
        p.extract(headers.data);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    apply {
        truncate((bit<32>) headers.spec.bytes_to_save);
        // modify header to be able to check whether it is emitted
        headers.data.data = 32w0xFF_FE_FD_FC;
    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.data);
    }
}

ubpf(prs(), pipe(), dprs()) main;