#include <core.p4>
#include <v1model.p4>

struct user_metadata_t {
    bit<32> pkt_count;
    bit<2>  status_cycle;
    bit<64> global_count;
}

struct metadata {
    bit<32> _md_pkt_count0;
    bit<2>  _md_status_cycle1;
    bit<64> _md_global_count2;
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
        meta._md_status_cycle1 = (bit<2>)(meta._md_pkt_count0 << 2);
    }
    @name(".incr_global_count") action incr_global_count() {
        meta._md_global_count2 = (bit<64>)meta._md_pkt_count0 << 2;
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

