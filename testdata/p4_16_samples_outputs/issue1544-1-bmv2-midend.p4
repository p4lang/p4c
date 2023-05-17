#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.smeta") standard_metadata_t smeta_0;
    @name("ingress.tmp") bit<16> tmp_0;
    @name(".my_drop") action my_drop_0() {
        smeta_0.ingress_port = standard_metadata.ingress_port;
        smeta_0.egress_spec = standard_metadata.egress_spec;
        smeta_0.egress_port = standard_metadata.egress_port;
        smeta_0.instance_type = standard_metadata.instance_type;
        smeta_0.packet_length = standard_metadata.packet_length;
        smeta_0.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_0.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_0.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_0.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_0.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_0.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_0.mcast_grp = standard_metadata.mcast_grp;
        smeta_0.egress_rid = standard_metadata.egress_rid;
        smeta_0.checksum_error = standard_metadata.checksum_error;
        smeta_0.parser_error = standard_metadata.parser_error;
        smeta_0.priority = standard_metadata.priority;
        mark_to_drop(smeta_0);
        standard_metadata.ingress_port = smeta_0.ingress_port;
        standard_metadata.egress_spec = smeta_0.egress_spec;
        standard_metadata.egress_port = smeta_0.egress_port;
        standard_metadata.instance_type = smeta_0.instance_type;
        standard_metadata.packet_length = smeta_0.packet_length;
        standard_metadata.enq_timestamp = smeta_0.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_0.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_0.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_0.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_0.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_0.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_0.mcast_grp;
        standard_metadata.egress_rid = smeta_0.egress_rid;
        standard_metadata.checksum_error = smeta_0.checksum_error;
        standard_metadata.parser_error = smeta_0.parser_error;
        standard_metadata.priority = smeta_0.priority;
    }
    @name("ingress.set_port") action set_port(@name("output_port") bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
        }
        actions = {
            set_port();
            my_drop_0();
        }
        default_action = my_drop_0();
    }
    @hidden action issue15441bmv2l25() {
        tmp_0 = hdr.ethernet.srcAddr[15:0] + 16w65535;
    }
    @hidden action issue15441bmv2l27() {
        tmp_0 = hdr.ethernet.srcAddr[15:0];
    }
    @hidden action issue15441bmv2l83() {
        hdr.ethernet.srcAddr[15:0] = tmp_0;
    }
    @hidden table tbl_issue15441bmv2l25 {
        actions = {
            issue15441bmv2l25();
        }
        const default_action = issue15441bmv2l25();
    }
    @hidden table tbl_issue15441bmv2l27 {
        actions = {
            issue15441bmv2l27();
        }
        const default_action = issue15441bmv2l27();
    }
    @hidden table tbl_issue15441bmv2l83 {
        actions = {
            issue15441bmv2l83();
        }
        const default_action = issue15441bmv2l83();
    }
    apply {
        mac_da_0.apply();
        if (hdr.ethernet.srcAddr[15:0] > 16w5) {
            tbl_issue15441bmv2l25.apply();
        } else {
            tbl_issue15441bmv2l27.apply();
        }
        tbl_issue15441bmv2l83.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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
