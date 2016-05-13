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
    OverwritingHeader
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> sizeInBits);
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
    Packets,
    Bytes,
    Both
}

extern Counter {
    Counter(bit<32> size, CounterType type);
    void increment(in bit<32> index);
}

extern DirectCounter {
    DirectCounter(CounterType type);
}

extern Meter {
    Meter(bit<32> size, CounterType type);
    void meter<T>(in bit<32> index, out T result);
}

extern DirectMeter<T> {
    DirectMeter(CounterType type);
    void read(out T result);
}

extern Register<T> {
    Register(bit<32> size);
    void read(out T result, in bit<32> index);
    void write(in bit<32> index, in T value);
}

extern ActionProfile {
    ActionProfile(bit<32> size);
}

enum HashAlgorithm {
    crc32,
    crc16,
    random,
    identity
}

extern ActionSelector {
    ActionSelector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

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
    bit<1>  ipv4_unicast_enabled;
    bit<1>  ipv6_unicast_enabled;
    bit<2>  ipv4_multicast_mode;
    bit<2>  ipv6_multicast_mode;
    bit<1>  igmp_snooping_enabled;
    bit<1>  mld_snooping_enabled;
    bit<2>  ipv4_urpf_mode;
    bit<2>  ipv6_urpf_mode;
    bit<10> rmac_group;
    bit<16> bd_mrpf_group;
    bit<16> uuc_mc_index;
    bit<16> umc_mc_index;
    bit<16> bcast_mc_index;
    bit<16> bd_label;
}

struct intrinsic_metadata_t {
    bit<16> exclusion_id1;
}

header data_t {
    bit<16> f1;
    bit<16> f2;
}

struct metadata {
    @name("ingress_metadata") 
    ingress_metadata_t   ingress_metadata;
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
}

struct headers {
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.data);
        meta.ingress_metadata.bd = hdr.data.f2;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("set_bd_info") action set_bd_info_0(bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<16> exclusion_id) {
        meta.ingress_metadata.vrf = vrf;
        meta.ingress_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ingress_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ingress_metadata.ipv4_multicast_mode = ipv4_multicast_mode;
        meta.ingress_metadata.ipv6_multicast_mode = ipv6_multicast_mode;
        meta.ingress_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
        meta.ingress_metadata.mld_snooping_enabled = mld_snooping_enabled;
        meta.ingress_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.ingress_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.ingress_metadata.rmac_group = rmac_group;
        meta.ingress_metadata.bd_mrpf_group = mrpf_group;
        meta.ingress_metadata.uuc_mc_index = uuc_mc_index;
        meta.ingress_metadata.umc_mc_index = umc_mc_index;
        meta.ingress_metadata.bcast_mc_index = bcast_mc_index;
        meta.ingress_metadata.bd_label = bd_label;
        meta.intrinsic_metadata.exclusion_id1 = exclusion_id;
    }
    @name("bd") table bd_0() {
        actions = {
            set_bd_info_0;
            NoAction_0;
        }
        key = {
            meta.ingress_metadata.bd: exact;
        }
        size = 16384;
        default_action = NoAction_0();
    }
    apply {
        bd_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
