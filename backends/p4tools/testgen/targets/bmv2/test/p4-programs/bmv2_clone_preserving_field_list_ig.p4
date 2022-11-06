#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
    bit<32> clone_val;
}

header clone_header_t {
    bit<32> pkt_len;
}

struct headers_t {
    ethernet_t ethernet;
    clone_header_t c;
}

struct metadata_t {
    @field_list(0)
    bool is_recirculated;
    bool is_recirculated_without_anno;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (!meta.is_recirculated) {
            clone_preserving_field_list(CloneType.I2E, 1, 0);
            meta.is_recirculated = true;
            meta.is_recirculated_without_anno = true;
            hdr.ethernet.src_addr = 0xFFFFFFFFFFFF;
        }
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        hdr.ethernet.ethertype = 0xAAAA;
        // This should have no effect on the ether type because we do not preserve the metadata.
        if (meta.is_recirculated_without_anno) {
            hdr.ethernet.ethertype = 0xBBBB;
        }
        if (standard_metadata.instance_type == 1) {
            hdr.ethernet.clone_val = 0xCCCCCCCC;
            hdr.c.setValid();
            hdr.c.pkt_len = standard_metadata.packet_length;
        }
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr);
    }
}


control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

