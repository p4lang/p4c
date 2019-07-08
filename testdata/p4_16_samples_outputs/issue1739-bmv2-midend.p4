#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddressUint_t;
typedef EthernetAddressUint_t EthernetAddress_t;
typedef bit<32> IPv4AddressUint_t;
typedef IPv4AddressUint_t IPv4Address_t;
typedef IPv4Address_t IPv4Address2_t;
header ethernet_t {
    EthernetAddress_t dstAddr;
    EthernetAddress_t srcAddr;
    bit<16>           etherType;
}

header ipv4_t {
    bit<4>         version;
    bit<4>         ihl;
    bit<8>         diffserv;
    bit<16>        totalLen;
    bit<16>        identification;
    bit<3>         flags;
    bit<13>        fragOffset;
    bit<8>         ttl;
    bit<8>         protocol;
    bit<16>        hdrChecksum;
    IPv4Address2_t srcAddr;
    IPv4Address_t  dstAddr;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    standard_metadata_t smeta;
    standard_metadata_t smeta_1;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.set_output") action set_output(bit<9> out_port) {
        standard_metadata.egress_spec = out_port;
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_output();
            my_drop();
        }
        default_action = my_drop();
    }
    @name("ingress.ipv4_sa_filter") table ipv4_sa_filter_0 {
        key = {
            hdr.ipv4.srcAddr: ternary @name("hdr.ipv4.srcAddr") ;
        }
        actions = {
            my_drop_0();
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_sa_filter_0.apply();
            ipv4_da_lpm_0.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
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

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

