#include <core.p4>
#include <ubpf_model.p4>

header truncate_spec_t {
    bit<8> bytes_to_save;
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
        p.extract<truncate_spec_t>(headers.spec);
        p.extract<next_header_t>(headers.data);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @hidden action truncate_ubpf31() {
        truncate((bit<32>)headers.spec.bytes_to_save);
        headers.data.data = 32w0xfffefdfc;
    }
    @hidden table tbl_truncate_ubpf31 {
        actions = {
            truncate_ubpf31();
        }
        const default_action = truncate_ubpf31();
    }
    apply {
        tbl_truncate_ubpf31.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action truncate_ubpf40() {
        packet.emit<next_header_t>(headers.data);
    }
    @hidden table tbl_truncate_ubpf40 {
        actions = {
            truncate_ubpf40();
        }
        const default_action = truncate_ubpf40();
    }
    apply {
        tbl_truncate_ubpf40.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
