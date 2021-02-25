#include <core.p4>
#include <v1model.p4>

struct headers_t {
}

struct metadata_t {
   varbit<8> test;
}

parser ParserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t standard_metadata) {
    state start {
        transition select() {
            default: accept;
        }
    }
}

control IngressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control EgressImpl(inout headers_t hdr,
                   inout metadata_t meta,
                   inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

V1Switch(ParserImpl(),
         VerifyChecksumImpl(),
         IngressImpl(),
         EgressImpl(),
         ComputeChecksumImpl(),
         DeparserImpl()) main;
