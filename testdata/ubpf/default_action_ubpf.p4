#include <core.p4>
#include <ubpf_model.p4>

header hdr_t {
    bit<32> a;
    bit<32> b;
}

struct Headers_t {
    hdr_t h;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.h);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action add(bit<32> data)
        { headers.h.b = headers.h.a + data; }

    table tbl_a {
        actions = { add; }
        default_action = add(10);
    }

    apply {
        tbl_a.apply();
    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply { packet.emit(headers.h); }
}

ubpf(prs(), pipe(), dprs()) main;