struct Version {
    bit<8> major;
    bit<8> minor;
}

error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader,
    HeaderTooShort
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

match_kind {
    exact,
    ternary,
    lpm
}

match_kind {
    range,
    selector
}

struct standard_metadata_t {
    bit<9>  ingress_port;
    bit<9>  egress_spec;
    bit<9>  egress_port;
    bit<32> clone_spec;
    bit<32> instance_type;
    bit<1>  drop;
    bit<16> recirculate_port;
    bit<32> packet_length;
}

extern Checksum16 {
    bit<16> get<D>(in D data);
}

enum CounterType {
    packets,
    bytes,
    packets_and_bytes
}

extern counter {
    counter(bit<32> size, CounterType type);
    void count(in bit<32> index);
}

extern direct_counter {
    direct_counter(CounterType type);
}

extern meter {
    meter(bit<32> size, CounterType type);
    void execute_meter<T>(in bit<32> index, out T result);
}

extern direct_meter<T> {
    direct_meter(CounterType type);
    void read(out T result);
}

extern register<T> {
    register(bit<32> size);
    void read(out T result, in bit<32> index);
    void write(in bit<32> index, in T value);
}

extern action_profile {
    action_profile(bit<32> size);
}

enum HashAlgorithm {
    crc32,
    crc32_custom,
    crc16,
    crc16_custom,
    random,
    identity
}

extern void mark_to_drop();
extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);
extern action_selector {
    action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(in H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeCkecksum<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg, ComputeCkecksum<H, M> ck, Deparser<H> dep);
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
    action NoAction_2() {
    }
    @name("rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("send_frame") table send_frame_0() {
        actions = {
            rewrite_mac();
            _drop();
            NoAction_2();
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 256;
        default_action = NoAction_2();
    }
    apply {
        send_frame_0.apply();
    }
}

struct struct_0 {
    bit<32> field;
    bit<32> field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<16> field_4;
}

struct struct_1 {
    bit<32> field_5;
    bit<32> field_6;
    bit<8>  field_7;
    bit<16> field_8;
    bit<16> field_9;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    action NoAction_7() {
    }
    @name("flowlet_id") register<bit<16>>(32w8192) flowlet_id_0;
    @name("flowlet_lasttime") register<bit<32>>(32w8192) flowlet_lasttime_0;
    @name("_drop") action _drop_2() {
        mark_to_drop();
    }
    @name("_drop") action _drop_3() {
        mark_to_drop();
    }
    @name("_drop") action _drop_4() {
        mark_to_drop();
    }
    @name("set_ecmp_select") action set_ecmp_select(bit<8> ecmp_base, bit<8> ecmp_count) {
        hash<bit<14>, bit<10>, struct_0, bit<20>>(meta.ingress_metadata.ecmp_offset, HashAlgorithm.crc16, (bit<10>)ecmp_base, { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort, meta.ingress_metadata.flowlet_id }, (bit<20>)ecmp_count);
    }
    @name("set_nhop") action set_nhop(bit<32> nhop_ipv4, bit<9> port) {
        meta.ingress_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("lookup_flowlet_map") action lookup_flowlet_map() {
        hash<bit<13>, bit<13>, struct_1, bit<26>>(meta.ingress_metadata.flowlet_map_index, HashAlgorithm.crc16, 13w0, { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort }, 26w13);
        flowlet_id_0.read(meta.ingress_metadata.flowlet_id, (bit<32>)meta.ingress_metadata.flowlet_map_index);
        meta.ingress_metadata.flow_ipg = (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp;
        flowlet_lasttime_0.read(meta.ingress_metadata.flowlet_lasttime, (bit<32>)meta.ingress_metadata.flowlet_map_index);
        meta.ingress_metadata.flow_ipg = meta.ingress_metadata.flow_ipg - meta.ingress_metadata.flowlet_lasttime;
        flowlet_lasttime_0.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<32>)meta.intrinsic_metadata.ingress_global_timestamp);
    }
    @name("set_dmac") action set_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("update_flowlet_id") action update_flowlet_id() {
        meta.ingress_metadata.flowlet_id = meta.ingress_metadata.flowlet_id + 16w1;
        flowlet_id_0.write((bit<32>)meta.ingress_metadata.flowlet_map_index, (bit<16>)meta.ingress_metadata.flowlet_id);
    }
    @name("ecmp_group") table ecmp_group_0() {
        actions = {
            _drop_2();
            set_ecmp_select();
            NoAction_3();
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
        default_action = NoAction_3();
    }
    @name("ecmp_nhop") table ecmp_nhop_0() {
        actions = {
            _drop_3();
            set_nhop();
            NoAction_4();
        }
        key = {
            meta.ingress_metadata.ecmp_offset: exact;
        }
        size = 16384;
        default_action = NoAction_4();
    }
    @name("flowlet") table flowlet_0() {
        actions = {
            lookup_flowlet_map();
            NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name("forward") table forward_0() {
        actions = {
            set_dmac();
            _drop_4();
            NoAction_6();
        }
        key = {
            meta.ingress_metadata.nhop_ipv4: exact;
        }
        size = 512;
        default_action = NoAction_6();
    }
    @name("new_flowlet") table new_flowlet_0() {
        actions = {
            update_flowlet_id();
            NoAction_7();
        }
        default_action = NoAction_7();
    }
    apply {
        flowlet_0.apply();
        if (meta.ingress_metadata.flow_ipg > 32w50000) 
            new_flowlet_0.apply();
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

struct struct_2 {
    bit<4>  field_10;
    bit<4>  field_11;
    bit<8>  field_12;
    bit<16> field_13;
    bit<16> field_14;
    bit<3>  field_15;
    bit<13> field_16;
    bit<8>  field_17;
    bit<8>  field_18;
    bit<32> field_19;
    bit<32> field_20;
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<16> tmp;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_0;
    action act() {
        mark_to_drop();
    }
    action act_0() {
        tmp = ipv4_checksum_0.get<struct_2>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        if (hdr.ipv4.hdrChecksum == tmp) 
            tbl_act_0.apply();
    }
}

struct struct_3 {
    bit<4>  field_21;
    bit<4>  field_22;
    bit<8>  field_23;
    bit<16> field_24;
    bit<16> field_25;
    bit<3>  field_26;
    bit<13> field_27;
    bit<8>  field_28;
    bit<8>  field_29;
    bit<32> field_30;
    bit<32> field_31;
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ipv4_checksum") Checksum16() ipv4_checksum_1;
    action act_1() {
        hdr.ipv4.hdrChecksum = ipv4_checksum_1.get<struct_3>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
    table tbl_act_1() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        tbl_act_1.apply();
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
