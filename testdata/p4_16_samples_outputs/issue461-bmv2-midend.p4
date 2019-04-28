#include <core.p4>
#include <v1model.p4>

struct fwd_metadata_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct metadata {
    bit<32> _fwd_metadata_l2ptr0;
    bit<24> _fwd_metadata_out_bd1;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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
    @name("ingress.ipv4_da_lpm_stats") direct_counter(CounterType.packets) ipv4_da_lpm_stats_0;
    @name("ingress.set_l2ptr") action set_l2ptr(bit<32> l2ptr) {
        ipv4_da_lpm_stats_0.count();
        meta._fwd_metadata_l2ptr0 = l2ptr;
    }
    @name("ingress.drop_with_count") action drop_with_count() {
        ipv4_da_lpm_stats_0.count();
        mark_to_drop(standard_metadata);
    }
    @name("ingress.set_bd_dmac_intf") action set_bd_dmac_intf(bit<24> bd, bit<48> dmac, bit<9> intf) {
        meta._fwd_metadata_out_bd1 = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm_0 {
        actions = {
            set_l2ptr();
            drop_with_count();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        default_action = drop_with_count();
        counters = ipv4_da_lpm_stats_0;
    }
    @name("ingress.mac_da") table mac_da_0 {
        actions = {
            set_bd_dmac_intf();
            my_drop();
        }
        key = {
            meta._fwd_metadata_l2ptr0: exact @name("meta.fwd_metadata.l2ptr") ;
        }
        default_action = my_drop();
    }
    apply {
        ipv4_da_lpm_0.apply();
        mac_da_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    standard_metadata_t smeta_1;
    @name(".my_drop") action my_drop_0() {
        smeta_1.ingress_port = standard_metadata.ingress_port;
        smeta_1.egress_spec = standard_metadata.egress_spec;
        smeta_1.egress_port = standard_metadata.egress_port;
        smeta_1.instance_type = standard_metadata.instance_type;
        smeta_1.packet_length = standard_metadata.packet_length;
        smeta_1.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_1.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_1.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_1.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_1.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_1.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_1.mcast_grp = standard_metadata.mcast_grp;
        smeta_1.egress_rid = standard_metadata.egress_rid;
        smeta_1.checksum_error = standard_metadata.checksum_error;
        smeta_1.parser_error = standard_metadata.parser_error;
        smeta_1.priority = standard_metadata.priority;
        mark_to_drop(smeta_1);
        standard_metadata.ingress_port = smeta_1.ingress_port;
        standard_metadata.egress_spec = smeta_1.egress_spec;
        standard_metadata.egress_port = smeta_1.egress_port;
        standard_metadata.instance_type = smeta_1.instance_type;
        standard_metadata.packet_length = smeta_1.packet_length;
        standard_metadata.enq_timestamp = smeta_1.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_1.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_1.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_1.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_1.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_1.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_1.mcast_grp;
        standard_metadata.egress_rid = smeta_1.egress_rid;
        standard_metadata.checksum_error = smeta_1.checksum_error;
        standard_metadata.parser_error = smeta_1.parser_error;
        standard_metadata.priority = smeta_1.priority;
    }
    @name("egress.rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egress.send_frame") table send_frame_0 {
        actions = {
            rewrite_mac();
            my_drop_0();
        }
        key = {
            meta._fwd_metadata_out_bd1: exact @name("meta.fwd_metadata.out_bd") ;
        }
        default_action = my_drop_0();
    }
    apply {
        send_frame_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

struct tuple_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

