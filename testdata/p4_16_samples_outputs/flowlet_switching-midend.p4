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
    @name("NoAction_2") action NoAction() {
    }
    @name("rewrite_mac") action rewrite_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("_drop") action _drop_0() {
        mark_to_drop();
    }
    @name("send_frame") table send_frame() {
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
        send_frame.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<14> tmp_14;
    tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>, bit<16>> tmp_15;
    bit<8> tmp_16;
    bit<13> tmp_17;
    tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>> tmp_18;
    bit<16> tmp_19;
    bit<32> tmp_20;
    bit<32> tmp_21;
    bit<32> tmp_22;
    bit<32> tmp_23;
    bit<16> tmp_24;
    bool tmp_25;
    @name("NoAction_3") action NoAction_0() {
    }
    @name("NoAction_4") action NoAction_1() {
    }
    @name("NoAction_5") action NoAction_8() {
    }
    @name("NoAction_6") action NoAction_9() {
    }
    @name("NoAction_7") action NoAction_10() {
    }
    @name("flowlet_id") register<bit<16>>(32w8192) flowlet_id_1;
    @name("flowlet_lasttime") register<bit<32>>(32w8192) flowlet_lasttime_1;
    @name("_drop") action _drop_1() {
        mark_to_drop();
    }
    @name("_drop") action _drop_5() {
        mark_to_drop();
    }
    @name("_drop") action _drop_6() {
        mark_to_drop();
    }
    @name("set_ecmp_select") action set_ecmp_select_0(bit<8> ecmp_base, bit<8> ecmp_count) {
        tmp_15 = { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort, meta.ingress_metadata.flowlet_id };
        hash<bit<14>, bit<10>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>, bit<16>>, bit<20>>(tmp_14, HashAlgorithm.crc16, (bit<10>)ecmp_base, tmp_15, (bit<20>)ecmp_count);
        meta.ingress_metadata.ecmp_offset = tmp_14;
    }
    @name("set_nhop") action set_nhop_0(bit<32> nhop_ipv4, bit<9> port) {
        meta.ingress_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        tmp_16 = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("lookup_flowlet_map") action lookup_flowlet_map_0() {
        tmp_18 = { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort };
        hash<bit<13>, bit<13>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<26>>(tmp_17, HashAlgorithm.crc16, 13w0, tmp_18, 26w13);
        meta.ingress_metadata.flowlet_map_index = tmp_17;
        tmp_20 = (bit<32>)meta.ingress_metadata.flowlet_map_index;
        flowlet_id_1.read(tmp_19, (bit<32>)meta.ingress_metadata.flowlet_map_index);
        meta.ingress_metadata.flowlet_id = tmp_19;
        meta.ingress_metadata.flow_ipg = (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp;
        tmp_22 = (bit<32>)meta.ingress_metadata.flowlet_map_index;
        flowlet_lasttime_1.read(tmp_21, (bit<32>)meta.ingress_metadata.flowlet_map_index);
        meta.ingress_metadata.flowlet_lasttime = tmp_21;
        tmp_23 = meta.ingress_metadata.flow_ipg - meta.ingress_metadata.flowlet_lasttime;
        meta.ingress_metadata.flow_ipg = meta.ingress_metadata.flow_ipg - meta.ingress_metadata.flowlet_lasttime;
        flowlet_lasttime_1.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp);
    }
    @name("set_dmac") action set_dmac_0(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("update_flowlet_id") action update_flowlet_id_0() {
        tmp_24 = meta.ingress_metadata.flowlet_id + 16w1;
        meta.ingress_metadata.flowlet_id = meta.ingress_metadata.flowlet_id + 16w1;
        flowlet_id_1.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<16>)meta.ingress_metadata.flowlet_id);
    }
    @name("ecmp_group") table ecmp_group() {
        actions = {
            _drop_1();
            set_ecmp_select_0();
            NoAction_0();
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
        default_action = NoAction_0();
    }
    @name("ecmp_nhop") table ecmp_nhop() {
        actions = {
            _drop_5();
            set_nhop_0();
            NoAction_1();
        }
        key = {
            meta.ingress_metadata.ecmp_offset: exact;
        }
        size = 16384;
        default_action = NoAction_1();
    }
    @name("flowlet") table flowlet() {
        actions = {
            lookup_flowlet_map_0();
            NoAction_8();
        }
        default_action = NoAction_8();
    }
    @name("forward") table forward() {
        actions = {
            set_dmac_0();
            _drop_6();
            NoAction_9();
        }
        key = {
            meta.ingress_metadata.nhop_ipv4: exact;
        }
        size = 512;
        default_action = NoAction_9();
    }
    @name("new_flowlet") table new_flowlet() {
        actions = {
            update_flowlet_id_0();
            NoAction_10();
        }
        default_action = NoAction_10();
    }
    action act() {
        tmp_25 = meta.ingress_metadata.flow_ipg > 32w50000;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        @atomic {
            flowlet.apply();
            tbl_act.apply();
            if (tmp_25) 
                new_flowlet.apply();
        }
        ecmp_group.apply();
        ecmp_nhop.apply();
        forward.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    bit<16> tmp_26;
    bool tmp_27;
    @name("ipv4_checksum") Checksum16() ipv4_checksum;
    action act_0() {
        mark_to_drop();
    }
    action act_1() {
        tmp_27 = hdr.ipv4.hdrChecksum == tmp_26;
    }
    table tbl_act_0() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tmp_26 = ipv4_checksum.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        tbl_act_0.apply();
        if (tmp_27) 
            tbl_act_1.apply();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    bit<16> tmp_28;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_2;
    action act_2() {
        hdr.ipv4.hdrChecksum = tmp_28;
    }
    table tbl_act_2() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tmp_28 = ipv4_checksum_2.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        tbl_act_2.apply();
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
