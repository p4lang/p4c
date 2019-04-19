#include <core.p4>
#include <v1model.p4>

header simple {
    bit<8> h;
}

struct my_headers_t {
    simple s;
}

struct my_metadata_t {
    error parser_error;
}

parser MyParser(packet_in packet, out my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.s);
        meta.parser_error = error.NoMatch;
        transition accept;
    }
}

control MyVerifyChecksum(inout my_headers_t hdr, inout my_metadata_t meta) {
    apply {
    }
}

control MyIngress(inout my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (meta.parser_error == error.NoMatch) 
            hdr.s.setInvalid();
    }
}

control MyEgress(inout my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout my_headers_t hdr, inout my_metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in my_headers_t hdr) {
    apply {
        packet.emit(hdr.s);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

