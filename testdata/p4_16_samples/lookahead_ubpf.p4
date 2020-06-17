#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

header header_one {
    bit<8> type;
    bit<8> data;
}
header header_two {
    bit<8>  type;
    bit<16> data;
}
header header_four {
    bit<8>  type;
    bit<32> data;
}

struct Headers_t {
    header_one one;
    header_two two;
    header_four four;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition parse_headers;
    }

    state parse_headers {
        transition select(p.lookahead<bit<8>>()) {
            1: parse_one;
            2: parse_two;
            4: parse_four;
            default: accept;
        }
    }

    state parse_one {
        p.extract(headers.one);
        transition parse_headers;
    }
    state parse_two {
        p.extract(headers.two);
        transition parse_headers;
    }
    state parse_four {
        p.extract(headers.four);
        transition parse_headers;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    apply { }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.one);
        packet.emit(headers.two);
        packet.emit(headers.four);
    }
}

ubpf(prs(), pipe(), dprs()) main;