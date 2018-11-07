#include <core.p4>
#include <v1model.p4>

struct custom_metadata_t {
    bit<32> nhop_ipv4;
    bit<16> hash_val1;
    bit<16> hash_val2;
    bit<16> count_val1;
    bit<16> count_val2;
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
    @name(".custom_metadata") 
    custom_metadata_t custom_metadata;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".send_frame") table send_frame_0 {
        actions = {
            rewrite_mac();
            _drop();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        size = 256;
        default_action = NoAction_0();
    }
    apply {
        send_frame_0.apply();
    }
}

@name(".heavy_hitter_counter1") register<bit<16>>(32w16) heavy_hitter_counter1;

@name(".heavy_hitter_counter2") register<bit<16>>(32w16) heavy_hitter_counter2;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_7() {
    }
    @name(".NoAction") action NoAction_8() {
    }
    @name(".NoAction") action NoAction_9() {
    }
    @name("._drop") action _drop_2() {
        mark_to_drop();
    }
    @name("._drop") action _drop_5() {
        mark_to_drop();
    }
    @name("._drop") action _drop_6() {
        mark_to_drop();
    }
    @name(".set_dmac") action set_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".set_nhop") action set_nhop(bit<32> nhop_ipv4, bit<9> port) {
        meta.custom_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".set_heavy_hitter_count") action set_heavy_hitter_count() {
        hash<bit<16>, bit<16>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.custom_metadata.hash_val1, HashAlgorithm.csum16, 16w0, { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort }, 32w16);
        heavy_hitter_counter1.read(meta.custom_metadata.count_val1, (bit<32>)meta.custom_metadata.hash_val1);
        meta.custom_metadata.count_val1 = meta.custom_metadata.count_val1 + 16w1;
        heavy_hitter_counter1.write((bit<32>)meta.custom_metadata.hash_val1, meta.custom_metadata.count_val1);
        hash<bit<16>, bit<16>, tuple<bit<32>, bit<32>, bit<8>, bit<16>, bit<16>>, bit<32>>(meta.custom_metadata.hash_val2, HashAlgorithm.crc16, 16w0, { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort }, 32w16);
        heavy_hitter_counter2.read(meta.custom_metadata.count_val2, (bit<32>)meta.custom_metadata.hash_val2);
        meta.custom_metadata.count_val2 = meta.custom_metadata.count_val2 + 16w1;
        heavy_hitter_counter2.write((bit<32>)meta.custom_metadata.hash_val2, meta.custom_metadata.count_val2);
    }
    @name(".drop_heavy_hitter_table") table drop_heavy_hitter_table_0 {
        actions = {
            _drop_2();
            @defaultonly NoAction_1();
        }
        size = 1;
        default_action = NoAction_1();
    }
    @name(".forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_5();
            @defaultonly NoAction_7();
        }
        key = {
            meta.custom_metadata.nhop_ipv4: exact @name("custom_metadata.nhop_ipv4") ;
        }
        size = 512;
        default_action = NoAction_7();
    }
    @name(".ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_6();
            @defaultonly NoAction_8();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("ipv4.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_8();
    }
    @name(".set_heavy_hitter_count_table") table set_heavy_hitter_count_table_0 {
        actions = {
            set_heavy_hitter_count();
            @defaultonly NoAction_9();
        }
        size = 1;
        default_action = NoAction_9();
    }
    apply {
        set_heavy_hitter_count_table_0.apply();
        if (meta.custom_metadata.count_val1 > 16w100 && meta.custom_metadata.count_val2 > 16w100) 
            drop_heavy_hitter_table_0.apply();
        else {
            ipv4_lpm_0.apply();
            forward_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

