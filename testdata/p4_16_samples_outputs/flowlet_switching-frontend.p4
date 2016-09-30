#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<32> flow_ipg;
    bit<13> flowlet_map_index;
    bit<16> flowlet_id;
    bit<32> flowlet_lasttime;
    bit<14> ecmp_offset;
    bit<32> nhop_ipv4;
}

struct intrinsic_metadata_t {
    bit<48> ingress_global_timestamp;
    bit<32> lf_field_list;
    bit<16> mcast_grp;
    bit<16> egress_rid;
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

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct metadata {
    @name("ingress_metadata") 
    ingress_metadata_t   ingress_metadata;
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("tcp") 
    tcp_t      tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    @name("parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("rewrite_mac") action rewrite_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("_drop") action _drop_0() {
        mark_to_drop();
    }
    @name("send_frame") table send_frame_0() {
        actions = {
            rewrite_mac_0();
            _drop_0();
            NoAction();
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 256;
        default_action = NoAction();
    }
    apply {
        send_frame_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("flowlet_id") register<bit<16>>(32w8192) flowlet_id_0;
    @name("flowlet_lasttime") register<bit<32>>(32w8192) flowlet_lasttime_0;
    @name("_drop") action _drop_1() {
        mark_to_drop();
    }
    @name("set_ecmp_select") action set_ecmp_select_0(bit<8> ecmp_base, bit<8> ecmp_count) {
        bit<14> tmp;
        HashAlgorithm tmp_0;
        bit<10> tmp_1;
        tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>, bit<16>> tmp_2;
        bit<20> tmp_3;
        tmp_0 = HashAlgorithm.crc16;
        tmp_1 = (bit<10>)ecmp_base;
        tmp_2 = { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort, meta.ingress_metadata.flowlet_id };
        tmp_3 = (bit<20>)ecmp_count;
        hash<bit<14>, bit<10>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>, bit<16>>, bit<20>>(tmp, tmp_0, tmp_1, tmp_2, tmp_3);
        meta.ingress_metadata.ecmp_offset = tmp;
    }
    @name("set_nhop") action set_nhop_0(bit<32> nhop_ipv4, bit<9> port) {
        bit<8> tmp_4;
        meta.ingress_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        tmp_4 = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.ttl = tmp_4;
    }
    @name("lookup_flowlet_map") action lookup_flowlet_map_0() {
        bit<13> tmp_5;
        HashAlgorithm tmp_6;
        bit<13> tmp_7;
        tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>> tmp_8;
        bit<26> tmp_9;
        bit<16> tmp_10;
        bit<32> tmp_11;
        bit<32> tmp_12;
        bit<32> tmp_13;
        bit<32> tmp_14;
        tmp_6 = HashAlgorithm.crc16;
        tmp_7 = 13w0;
        tmp_8 = { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort };
        tmp_9 = 26w13;
        hash<bit<13>, bit<13>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<26>>(tmp_5, tmp_6, tmp_7, tmp_8, tmp_9);
        meta.ingress_metadata.flowlet_map_index = tmp_5;
        tmp_11 = (bit<32>)meta.ingress_metadata.flowlet_map_index;
        flowlet_id_0.read(tmp_10, tmp_11);
        meta.ingress_metadata.flowlet_id = tmp_10;
        meta.ingress_metadata.flow_ipg = (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp;
        tmp_13 = (bit<32>)meta.ingress_metadata.flowlet_map_index;
        flowlet_lasttime_0.read(tmp_12, tmp_13);
        meta.ingress_metadata.flowlet_lasttime = tmp_12;
        tmp_14 = meta.ingress_metadata.flow_ipg - meta.ingress_metadata.flowlet_lasttime;
        meta.ingress_metadata.flow_ipg = tmp_14;
        flowlet_lasttime_0.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp);
    }
    @name("set_dmac") action set_dmac_0(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("update_flowlet_id") action update_flowlet_id_0() {
        bit<16> tmp_15;
        tmp_15 = meta.ingress_metadata.flowlet_id + 16w1;
        meta.ingress_metadata.flowlet_id = tmp_15;
        flowlet_id_0.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<16>)meta.ingress_metadata.flowlet_id);
    }
    @name("ecmp_group") table ecmp_group_0() {
        actions = {
            _drop_1();
            set_ecmp_select_0();
            NoAction();
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name("ecmp_nhop") table ecmp_nhop_0() {
        actions = {
            _drop_1();
            set_nhop_0();
            NoAction();
        }
        key = {
            meta.ingress_metadata.ecmp_offset: exact;
        }
        size = 16384;
        default_action = NoAction();
    }
    @name("flowlet") table flowlet_0() {
        actions = {
            lookup_flowlet_map_0();
            NoAction();
        }
        default_action = NoAction();
    }
    @name("forward") table forward_0() {
        actions = {
            set_dmac_0();
            _drop_1();
            NoAction();
        }
        key = {
            meta.ingress_metadata.nhop_ipv4: exact;
        }
        size = 512;
        default_action = NoAction();
    }
    @name("new_flowlet") table new_flowlet_0() {
        actions = {
            update_flowlet_id_0();
            NoAction();
        }
        default_action = NoAction();
    }
    bool tmp_16;
    apply {
        @atomic {
            flowlet_0.apply();
            tmp_16 = meta.ingress_metadata.flow_ipg > 32w50000;
            if (tmp_16) 
                new_flowlet_0.apply();
        }
        ecmp_group_0.apply();
        ecmp_nhop_0.apply();
        forward_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ipv4_checksum") Checksum16() ipv4_checksum_0;
    bit<16> tmp_17;
    bool tmp_18;
    apply {
        tmp_17 = ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        tmp_18 = hdr.ipv4.hdrChecksum == tmp_17;
        if (tmp_18) 
            mark_to_drop();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ipv4_checksum") Checksum16() ipv4_checksum_1;
    bit<16> tmp_19;
    apply {
        tmp_19 = ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        hdr.ipv4.hdrChecksum = tmp_19;
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
