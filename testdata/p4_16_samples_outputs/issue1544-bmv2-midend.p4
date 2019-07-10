#include <core.p4>
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
    bit<16> retval;
    standard_metadata_t smeta;
    @name(".my_drop") action my_drop() {
        smeta.ingress_port = standard_metadata.ingress_port;
        smeta.egress_spec = standard_metadata.egress_spec;
        smeta.egress_port = standard_metadata.egress_port;
        smeta.instance_type = standard_metadata.instance_type;
        smeta.packet_length = standard_metadata.packet_length;
        smeta.enq_timestamp = standard_metadata.enq_timestamp;
        smeta.enq_qdepth = standard_metadata.enq_qdepth;
        smeta.deq_timedelta = standard_metadata.deq_timedelta;
        smeta.deq_qdepth = standard_metadata.deq_qdepth;
        smeta.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta.mcast_grp = standard_metadata.mcast_grp;
        smeta.egress_rid = standard_metadata.egress_rid;
        smeta.checksum_error = standard_metadata.checksum_error;
        smeta.parser_error = standard_metadata.parser_error;
        smeta.priority = standard_metadata.priority;
        mark_to_drop(smeta);
        standard_metadata.ingress_port = smeta.ingress_port;
        standard_metadata.egress_spec = smeta.egress_spec;
        standard_metadata.egress_port = smeta.egress_port;
        standard_metadata.instance_type = smeta.instance_type;
        standard_metadata.packet_length = smeta.packet_length;
        standard_metadata.enq_timestamp = smeta.enq_timestamp;
        standard_metadata.enq_qdepth = smeta.enq_qdepth;
        standard_metadata.deq_timedelta = smeta.deq_timedelta;
        standard_metadata.deq_qdepth = smeta.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta.mcast_grp;
        standard_metadata.egress_rid = smeta.egress_rid;
        standard_metadata.checksum_error = smeta.checksum_error;
        standard_metadata.parser_error = smeta.parser_error;
        standard_metadata.priority = smeta.priority;
    }
    @name("ingress.set_port") action set_port(bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            set_port();
            my_drop();
        }
        default_action = my_drop();
    }
    @hidden action act() {
        retval = hdr.ethernet.srcAddr[15:0] + 16w65535;
    }
    @hidden action act_0() {
        retval = hdr.ethernet.srcAddr[15:0];
    }
    @hidden action act_1() {
        hdr.ethernet.srcAddr[15:0] = retval;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        mac_da_0.apply();
        if (hdr.ethernet.srcAddr[15:0] > 16w5) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        tbl_act_1.apply();
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

