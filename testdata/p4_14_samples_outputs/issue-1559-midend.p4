#include <core.p4>
#include <v1model.p4>

struct user_metadata_t {
    bit<32> pkt_count;
    bit<2>  status_cycle;
    bit<64> global_count;
}

struct metadata {
    @name(".md") 
    user_metadata_t md;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".maintain_2bit_variable") action maintain_2bit_variable() {
        meta.md.status_cycle = (bit<2>)(meta.md.pkt_count << 2);
    }
    @name(".incr_global_count") action incr_global_count() {
        meta.md.global_count = (bit<64>)meta.md.pkt_count << 2;
    }
    @name(".tb_maintain_2bit_variable") table tb_maintain_2bit_variable_0 {
        actions = {
            maintain_2bit_variable();
            incr_global_count();
        }
        default_action = maintain_2bit_variable();
    }
    apply {
        tb_maintain_2bit_variable_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

