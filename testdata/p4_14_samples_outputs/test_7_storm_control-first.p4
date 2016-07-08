struct Version {
    bit<8> major;
    bit<8> minor;
}

const Version P4_VERSION = { 8w1, 8w2 };
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

extern void verify(in bool check, in error toSignal);
action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

const Version v1modelVersion = { 8w0, 8w1 };
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

extern bit<32> random(in bit<5> logRange);
extern void digest<T>(in bit<32> receiver, in T data);
enum HashAlgorithm {
    crc32,
    crc16,
    random,
    identity
}

extern void mark_to_drop();
extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);
extern action_selector {
    action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

enum CloneType {
    I2E,
    E2E
}

extern void resubmit<T>(in T data);
extern void recirculate<T>(in T data);
extern void clone(in CloneType type, in bit<32> session);
extern void clone3<T>(in CloneType type, in bit<32> session, in T data);
parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(in H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeCkecksum<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg, ComputeCkecksum<H, M> ck, Deparser<H> dep);
struct ingress_metadata_t {
    bit<16> bd;
    bit<12> vrf;
    bit<12> v6_vrf;
    bit<1>  ipv4_term;
    bit<1>  ipv6_term;
    bit<1>  igmp_snoop;
    bit<1>  tunnel_term;
    bit<32> tunnel_vni;
    bit<16> ing_meter;
    bit<16> i_lif;
    bit<16> i_tunnel_lif;
    bit<16> o_lif;
    bit<16> if_acl_label;
    bit<16> route_acl_label;
    bit<16> bd_flags;
    bit<8>  stp_instance;
    bit<1>  route;
    bit<1>  inner_route;
    bit<1>  l2_miss;
    bit<1>  l3_lpm_miss;
    bit<16> mc_index;
    bit<16> inner_mc_index;
    bit<16> nhop;
    bit<10> ecmp_index;
    bit<14> ecmp_offset;
    bit<16> nsh_value;
    bit<8>  lag_index;
    bit<15> lag_port;
    bit<14> lag_offset;
    bit<1>  flood;
    bit<1>  learn_type;
    bit<48> learn_mac;
    bit<32> learn_ipv4;
    bit<1>  mcast_drop;
    bit<1>  drop_2;
    bit<1>  drop_1;
    bit<1>  drop_0;
    bit<8>  drop_reason;
    bit<1>  copy_to_cpu;
    bit<10> mirror_sesion_id;
    bit<1>  urpf;
    bit<2>  urpf_mode;
    bit<16> urpf_strict;
    bit<1>  ingress_bypass;
    bit<24> ipv4_dstaddr_24b;
    bit<16> nhop_index;
    bit<1>  if_drop;
    bit<1>  route_drop;
    bit<32> ipv4_dest;
    bit<48> eth_dstAddr;
    bit<48> eth_srcAddr;
    bit<8>  ipv4_ttl;
    bit<3>  dcsp;
    bit<3>  buffer_qos;
    bit<16> ip_srcPort;
    bit<16> ip_dstPort;
    bit<1>  mcast_pkt;
    bit<1>  inner_mcast_pkt;
    bit<1>  if_copy_to_cpu;
    bit<16> if_nhop;
    bit<9>  if_port;
    bit<10> if_ecmp_index;
    bit<8>  if_lag_index;
    bit<1>  route_copy_to_cpu;
    bit<16> route_nhop;
    bit<9>  route_port;
    bit<10> route_ecmp_index;
    bit<8>  route_lag_index;
    bit<8>  tun_type;
    bit<1>  nat_enable;
    bit<13> nat_source_index;
    bit<13> nat_dest_index;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name("ingress_metadata") 
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("no_action") action no_action() {
    }
    @name("ing_meter_set") action ing_meter_set(bit<16> meter_) {
        meta.ingress_metadata.ing_meter = meter_;
    }
    @name("storm_control") table storm_control() {
        actions = {
            no_action();
            ing_meter_set();
            NoAction();
        }
        key = {
            meta.ingress_metadata.bd: exact;
            hdr.ethernet.dstAddr    : ternary;
        }
        size = 8192;
        default_action = NoAction();
    }
    apply {
        storm_control.apply();
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

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
