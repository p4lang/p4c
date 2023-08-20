#include <core.p4>
#define V1MODEL_VERSION 20200408
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
    bit<32> _ingress_metadata_flow_ipg0;
    bit<13> _ingress_metadata_flowlet_map_index1;
    bit<16> _ingress_metadata_flowlet_id2;
    bit<32> _ingress_metadata_flowlet_lasttime3;
    bit<14> _ingress_metadata_ecmp_offset4;
    bit<32> _ingress_metadata_nhop_ipv45;
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".ipv4")
    ipv4_t     ipv4;
    @name(".tcp")
    tcp_t      tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name(".rewrite_mac") action rewrite_mac(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".send_frame") table send_frame_0 {
        actions = {
            rewrite_mac();
            _drop();
            @defaultonly NoAction_2();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port");
        }
        size = 256;
        default_action = NoAction_2();
    }
    apply {
        send_frame_0.apply();
    }
}

@name(".flowlet_id") register<bit<16>, bit<13>>(32w8192) flowlet_id;
@name(".flowlet_lasttime") register<bit<32>, bit<13>>(32w8192) flowlet_lasttime;
struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<16> f5;
}

struct tuple_1 {
    bit<32> f0;
    bit<32> f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @name("._drop") action _drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_3() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_4() {
        mark_to_drop(standard_metadata);
    }
    @name(".set_ecmp_select") action set_ecmp_select(@name("ecmp_base") bit<8> ecmp_base, @name("ecmp_count") bit<8> ecmp_count) {
        hash<bit<14>, bit<10>, tuple_0, bit<20>>(meta._ingress_metadata_ecmp_offset4, HashAlgorithm.crc16, (bit<10>)ecmp_base, (tuple_0){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = hdr.ipv4.protocol,f3 = hdr.tcp.srcPort,f4 = hdr.tcp.dstPort,f5 = meta._ingress_metadata_flowlet_id2}, (bit<20>)ecmp_count);
    }
    @name(".set_nhop") action set_nhop(@name("nhop_ipv4") bit<32> nhop_ipv4_1, @name("port") bit<9> port) {
        meta._ingress_metadata_nhop_ipv45 = nhop_ipv4_1;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".lookup_flowlet_map") action lookup_flowlet_map() {
        hash<bit<13>, bit<13>, tuple_1, bit<26>>(meta._ingress_metadata_flowlet_map_index1, HashAlgorithm.crc16, 13w0, (tuple_1){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = hdr.ipv4.protocol,f3 = hdr.tcp.srcPort,f4 = hdr.tcp.dstPort}, 26w13);
        flowlet_id.read(meta._ingress_metadata_flowlet_id2, meta._ingress_metadata_flowlet_map_index1);
        meta._ingress_metadata_flow_ipg0 = (bit<32>)standard_metadata.ingress_global_timestamp;
        flowlet_lasttime.read(meta._ingress_metadata_flowlet_lasttime3, meta._ingress_metadata_flowlet_map_index1);
        meta._ingress_metadata_flow_ipg0 = (bit<32>)standard_metadata.ingress_global_timestamp - meta._ingress_metadata_flowlet_lasttime3;
        flowlet_lasttime.write(meta._ingress_metadata_flowlet_map_index1, (bit<32>)standard_metadata.ingress_global_timestamp);
    }
    @name(".set_dmac") action set_dmac(@name("dmac") bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".update_flowlet_id") action update_flowlet_id() {
        meta._ingress_metadata_flowlet_id2 = meta._ingress_metadata_flowlet_id2 + 16w1;
        flowlet_id.write(meta._ingress_metadata_flowlet_map_index1, meta._ingress_metadata_flowlet_id2);
    }
    @name(".ecmp_group") table ecmp_group_0 {
        actions = {
            _drop_2();
            set_ecmp_select();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("ipv4.dstAddr");
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name(".ecmp_nhop") table ecmp_nhop_0 {
        actions = {
            _drop_3();
            set_nhop();
            @defaultonly NoAction_4();
        }
        key = {
            meta._ingress_metadata_ecmp_offset4: exact @name("ingress_metadata.ecmp_offset");
        }
        size = 16384;
        default_action = NoAction_4();
    }
    @name(".flowlet") table flowlet_0 {
        actions = {
            lookup_flowlet_map();
            @defaultonly NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name(".forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_4();
            @defaultonly NoAction_6();
        }
        key = {
            meta._ingress_metadata_nhop_ipv45: exact @name("ingress_metadata.nhop_ipv4");
        }
        size = 512;
        default_action = NoAction_6();
    }
    @name(".new_flowlet") table new_flowlet_0 {
        actions = {
            update_flowlet_id();
            @defaultonly NoAction_7();
        }
        default_action = NoAction_7();
    }
    apply {
        flowlet_0.apply();
        if (meta._ingress_metadata_flow_ipg0 > 32w50000) {
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

struct tuple_2 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_2, bit<16>>(true, (tuple_2){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_2, bit<16>>(true, (tuple_2){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
