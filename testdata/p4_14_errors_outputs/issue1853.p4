#include <core.p4>
#include <v1model.p4>

header mpls_t {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

struct metadata {
}

struct headers {
    @name(".value") 
    mpls_t[5] value;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.value.next);
        transition select(hdr.value.last.bos) {
            1w0: start;
            1w1: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".clean_bos") action clean_bos() {
        hdr.value.last.bos = 1w0;
    }
    @name(".table_clean_bos") table table_clean_bos {
        actions = {
            clean_bos;
        }
        size = 1;
    }
    apply {
        table_clean_bos.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.value);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

