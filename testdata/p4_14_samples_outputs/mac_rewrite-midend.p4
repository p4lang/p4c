#include <core.p4>
#include <v1model.p4>

struct egress_metadata_t {
    bit<16> payload_length;
    bit<9>  smac_idx;
    bit<16> bd;
    bit<1>  inner_replica;
    bit<1>  replica;
    bit<48> mac_da;
    bit<1>  routed;
    bit<16> same_bd_check;
    bit<4>  header_count;
    bit<8>  drop_reason;
    bit<1>  egress_bypass;
    bit<8>  drop_exception;
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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

struct metadata {
    @name("egress_metadata")
    egress_metadata_t egress_metadata;
}

struct headers {
    @name("ethernet")
    ethernet_t ethernet;
    @name("ipv4")
    ipv4_t     ipv4;
    @name("ipv6")
    ipv6_t     ipv6;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name("parse_ipv6") state parse_ipv6 {
        packet.extract<ipv6_t>(hdr.ipv6);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ethernet_t tmp_2_ethernet;
    ipv4_t tmp_2_ipv4;
    ipv6_t tmp_2_ipv6;
    egress_metadata_t tmp_3_egress_metadata;
    standard_metadata_t tmp_4;
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_3() {
    }
    @name("do_setup") action do_setup_0(bit<9> idx, bit<1> routed) {
        meta.egress_metadata.mac_da = hdr.ethernet.dstAddr;
        meta.egress_metadata.smac_idx = idx;
        meta.egress_metadata.routed = routed;
    }
    @name("setup") table setup {
        actions = {
            do_setup_0();
            @default_only NoAction_0();
        }
        key = {
            hdr.ethernet.isValid(): exact @name("hdr.ethernet.isValid()") ;
        }
        default_action = NoAction_0();
    }
    @name("process_mac_rewrite.nop") action process_mac_rewrite_nop() {
    }
    @name("process_mac_rewrite.rewrite_ipv4_unicast_mac") action process_mac_rewrite_rewrite_ipv4_unicast_mac(bit<48> smac) {
        tmp_2_ethernet.srcAddr = smac;
        tmp_2_ethernet.dstAddr = tmp_3_egress_metadata.mac_da;
        tmp_2_ipv4.ttl = tmp_2_ipv4.ttl + 8w255;
    }
    @name("process_mac_rewrite.rewrite_ipv4_multicast_mac") action process_mac_rewrite_rewrite_ipv4_multicast_mac(bit<48> smac) {
        tmp_2_ethernet.srcAddr = smac;
        tmp_2_ethernet.dstAddr[47:23] = 25w0x0;
        tmp_2_ipv4.ttl = tmp_2_ipv4.ttl + 8w255;
    }
    @name("process_mac_rewrite.rewrite_ipv6_unicast_mac") action process_mac_rewrite_rewrite_ipv6_unicast_mac(bit<48> smac) {
        tmp_2_ethernet.srcAddr = smac;
        tmp_2_ethernet.dstAddr = tmp_3_egress_metadata.mac_da;
        tmp_2_ipv6.hopLimit = tmp_2_ipv6.hopLimit + 8w255;
    }
    @name("process_mac_rewrite.rewrite_ipv6_multicast_mac") action process_mac_rewrite_rewrite_ipv6_multicast_mac(bit<48> smac) {
        tmp_2_ethernet.srcAddr = smac;
        tmp_2_ethernet.dstAddr[47:32] = 16w0x0;
        tmp_2_ipv6.hopLimit = tmp_2_ipv6.hopLimit + 8w255;
    }
    @name("process_mac_rewrite.mac_rewrite") table process_mac_rewrite_mac_rewrite_0 {
        actions = {
            process_mac_rewrite_nop();
            process_mac_rewrite_rewrite_ipv4_unicast_mac();
            process_mac_rewrite_rewrite_ipv4_multicast_mac();
            process_mac_rewrite_rewrite_ipv6_unicast_mac();
            process_mac_rewrite_rewrite_ipv6_multicast_mac();
            @default_only NoAction_3();
        }
        key = {
            tmp_3_egress_metadata.smac_idx: exact @name("meta.egress_metadata.smac_idx") ;
            tmp_2_ipv4.isValid()          : exact @name("hdr.ipv4.isValid()") ;
            tmp_2_ipv6.isValid()          : exact @name("hdr.ipv6.isValid()") ;
        }
        size = 512;
        default_action = NoAction_3();
    }
    action act() {
        tmp_2_ethernet = hdr.ethernet;
        tmp_2_ipv4 = hdr.ipv4;
        tmp_2_ipv6 = hdr.ipv6;
        tmp_3_egress_metadata.payload_length = meta.egress_metadata.payload_length;
        tmp_3_egress_metadata.smac_idx = meta.egress_metadata.smac_idx;
        tmp_3_egress_metadata.bd = meta.egress_metadata.bd;
        tmp_3_egress_metadata.inner_replica = meta.egress_metadata.inner_replica;
        tmp_3_egress_metadata.replica = meta.egress_metadata.replica;
        tmp_3_egress_metadata.mac_da = meta.egress_metadata.mac_da;
        tmp_3_egress_metadata.routed = meta.egress_metadata.routed;
        tmp_3_egress_metadata.same_bd_check = meta.egress_metadata.same_bd_check;
        tmp_3_egress_metadata.header_count = meta.egress_metadata.header_count;
        tmp_3_egress_metadata.drop_reason = meta.egress_metadata.drop_reason;
        tmp_3_egress_metadata.egress_bypass = meta.egress_metadata.egress_bypass;
        tmp_3_egress_metadata.drop_exception = meta.egress_metadata.drop_exception;
        tmp_4.ingress_port = standard_metadata.ingress_port;
        tmp_4.egress_spec = standard_metadata.egress_spec;
        tmp_4.egress_port = standard_metadata.egress_port;
        tmp_4.clone_spec = standard_metadata.clone_spec;
        tmp_4.instance_type = standard_metadata.instance_type;
        tmp_4.drop = standard_metadata.drop;
        tmp_4.recirculate_port = standard_metadata.recirculate_port;
        tmp_4.packet_length = standard_metadata.packet_length;
    }
    action act_0() {
        hdr.ethernet = tmp_2_ethernet;
        hdr.ipv4 = tmp_2_ipv4;
        hdr.ipv6 = tmp_2_ipv6;
        meta.egress_metadata.payload_length = tmp_3_egress_metadata.payload_length;
        meta.egress_metadata.smac_idx = tmp_3_egress_metadata.smac_idx;
        meta.egress_metadata.bd = tmp_3_egress_metadata.bd;
        meta.egress_metadata.inner_replica = tmp_3_egress_metadata.inner_replica;
        meta.egress_metadata.replica = tmp_3_egress_metadata.replica;
        meta.egress_metadata.mac_da = tmp_3_egress_metadata.mac_da;
        meta.egress_metadata.routed = tmp_3_egress_metadata.routed;
        meta.egress_metadata.same_bd_check = tmp_3_egress_metadata.same_bd_check;
        meta.egress_metadata.header_count = tmp_3_egress_metadata.header_count;
        meta.egress_metadata.drop_reason = tmp_3_egress_metadata.drop_reason;
        meta.egress_metadata.egress_bypass = tmp_3_egress_metadata.egress_bypass;
        meta.egress_metadata.drop_exception = tmp_3_egress_metadata.drop_exception;
        standard_metadata.ingress_port = tmp_4.ingress_port;
        standard_metadata.egress_spec = tmp_4.egress_spec;
        standard_metadata.egress_port = tmp_4.egress_port;
        standard_metadata.clone_spec = tmp_4.clone_spec;
        standard_metadata.instance_type = tmp_4.instance_type;
        standard_metadata.drop = tmp_4.drop;
        standard_metadata.recirculate_port = tmp_4.recirculate_port;
        standard_metadata.packet_length = tmp_4.packet_length;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        setup.apply();
        tbl_act.apply();
        if (tmp_3_egress_metadata.routed == 1w1)
            process_mac_rewrite_mac_rewrite_0.apply();
        tbl_act_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
