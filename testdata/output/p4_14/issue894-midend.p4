#include <core.p4>
#define V1MODEL_VERSION 20200408
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
    bit<32> _custom_metadata_nhop_ipv40;
    bit<16> _custom_metadata_hash_val11;
    bit<16> _custom_metadata_hash_val22;
    bit<16> _custom_metadata_count_val13;
    bit<16> _custom_metadata_count_val24;
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

@name(".heavy_hitter_counter1") register<bit<16>, bit<4>>(32w16) heavy_hitter_counter1;
@name(".heavy_hitter_counter2") register<bit<16>, bit<4>>(32w16) heavy_hitter_counter2;
struct tuple_0 {
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
    @name("._drop") action _drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_3() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_4() {
        mark_to_drop(standard_metadata);
    }
    @name(".set_dmac") action set_dmac(@name("dmac") bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".set_nhop") action set_nhop(@name("nhop_ipv4") bit<32> nhop_ipv4_1, @name("port") bit<9> port) {
        meta._custom_metadata_nhop_ipv40 = nhop_ipv4_1;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".set_heavy_hitter_count") action set_heavy_hitter_count() {
        hash<bit<16>, bit<16>, tuple_0, bit<32>>(meta._custom_metadata_hash_val11, HashAlgorithm.csum16, 16w0, (tuple_0){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = hdr.ipv4.protocol,f3 = hdr.tcp.srcPort,f4 = hdr.tcp.dstPort}, 32w16);
        heavy_hitter_counter1.read(meta._custom_metadata_count_val13, (bit<4>)meta._custom_metadata_hash_val11);
        meta._custom_metadata_count_val13 = meta._custom_metadata_count_val13 + 16w1;
        heavy_hitter_counter1.write((bit<4>)meta._custom_metadata_hash_val11, meta._custom_metadata_count_val13);
        hash<bit<16>, bit<16>, tuple_0, bit<32>>(meta._custom_metadata_hash_val22, HashAlgorithm.crc16, 16w0, (tuple_0){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = hdr.ipv4.protocol,f3 = hdr.tcp.srcPort,f4 = hdr.tcp.dstPort}, 32w16);
        heavy_hitter_counter2.read(meta._custom_metadata_count_val24, (bit<4>)meta._custom_metadata_hash_val22);
        meta._custom_metadata_count_val24 = meta._custom_metadata_count_val24 + 16w1;
        heavy_hitter_counter2.write((bit<4>)meta._custom_metadata_hash_val22, meta._custom_metadata_count_val24);
    }
    @name(".drop_heavy_hitter_table") table drop_heavy_hitter_table_0 {
        actions = {
            _drop_2();
            @defaultonly NoAction_3();
        }
        size = 1;
        default_action = NoAction_3();
    }
    @name(".forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_3();
            @defaultonly NoAction_4();
        }
        key = {
            meta._custom_metadata_nhop_ipv40: exact @name("custom_metadata.nhop_ipv4");
        }
        size = 512;
        default_action = NoAction_4();
    }
    @name(".ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_4();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("ipv4.dstAddr");
        }
        size = 1024;
        default_action = NoAction_5();
    }
    @name(".set_heavy_hitter_count_table") table set_heavy_hitter_count_table_0 {
        actions = {
            set_heavy_hitter_count();
            @defaultonly NoAction_6();
        }
        size = 1;
        default_action = NoAction_6();
    }
    apply {
        set_heavy_hitter_count_table_0.apply();
        if (meta._custom_metadata_count_val13 > 16w100 && meta._custom_metadata_count_val24 > 16w100) {
            drop_heavy_hitter_table_0.apply();
        } else {
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

struct tuple_1 {
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
        verify_checksum<tuple_1, bit<16>>(true, (tuple_1){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_1, bit<16>>(true, (tuple_1){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
