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
        p.extract<hdr_t>(headers.h);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.add") action add(@name("data") bit<32> data_1) {
        headers.h.b = headers.h.a + data_1;
    }
    @name("pipe.tbl_a") table tbl_a_0 {
        actions = {
            add();
        }
        default_action = add(32w10);
    }
    apply {
        tbl_a_0.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<hdr_t>(headers.h);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
