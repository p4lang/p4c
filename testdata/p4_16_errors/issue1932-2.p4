#include <core.p4>
#include <v1model.p4>

header h1_t { bit<8> f1; }
struct headers_t { h1_t h1; }
struct metadata_t { }

action foo (in bit<8> x, out bit<8> y) { y = (x >> 2); }
action foo (inout bit<8> x) { x = (x >> 3); }

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        foo(hdr.h1.f1);
        foo(hdr.h1.f1, hdr.h1.f1);
    }
}
parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) { state start { transition accept; } }
control verifyChecksum(inout headers_t hdr, inout metadata_t meta) { apply { } }
control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) { apply { } }
control updateChecksum(inout headers_t hdr, inout metadata_t meta) { apply { } }
control deparserImpl(packet_out packet, in headers_t hdr) { apply { } }
V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
