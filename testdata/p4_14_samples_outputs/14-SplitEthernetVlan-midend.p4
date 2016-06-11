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
    crc16,
    random,
    identity
}

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
header cfi_t {
    bit<1> cfi;
}

header len_or_type_t {
    bit<16> value;
}

header mac_da_t {
    bit<48> mac;
}

header mac_sa_t {
    bit<48> mac;
}

header pcp_t {
    bit<3> pcp;
}

header vlan_id_t {
    bit<12> vlan_id_t;
}

struct metadata {
}

struct headers {
    @name("cfi") 
    cfi_t         cfi;
    @name("len_or_type") 
    len_or_type_t len_or_type;
    @name("mac_da") 
    mac_da_t      mac_da;
    @name("mac_sa") 
    mac_sa_t      mac_sa;
    @name("pcp") 
    pcp_t         pcp;
    @name("vlan_id") 
    vlan_id_t     vlan_id;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_cfi") state parse_cfi {
        packet.extract(hdr.cfi);
        transition parse_vlan_id;
    }
    @name("parse_len_or_type") state parse_len_or_type {
        packet.extract(hdr.len_or_type);
        transition parse_pcp;
    }
    @name("parse_mac_da") state parse_mac_da {
        packet.extract(hdr.mac_da);
        transition parse_mac_sa;
    }
    @name("parse_mac_sa") state parse_mac_sa {
        packet.extract(hdr.mac_sa);
        transition parse_len_or_type;
    }
    @name("parse_pcp") state parse_pcp {
        packet.extract(hdr.pcp);
        transition parse_cfi;
    }
    @name("parse_vlan_id") state parse_vlan_id {
        packet.extract(hdr.vlan_id);
        transition accept;
    }
    @name("start") state start {
        transition parse_mac_da;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_2() {
    }
    @name("nop") action nop() {
    }
    @name("t2") table t2_0() {
        actions = {
            nop();
            NoAction_2();
        }
        key = {
            hdr.mac_sa.mac: exact;
        }
        default_action = NoAction_2();
    }
    apply {
        t2_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_3() {
    }
    @name("nop") action nop_2() {
    }
    @name("t1") table t1_0() {
        actions = {
            nop_2();
            NoAction_3();
        }
        key = {
            hdr.mac_da.mac       : exact;
            hdr.len_or_type.value: exact;
        }
        default_action = NoAction_3();
    }
    apply {
        t1_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.mac_da);
        packet.emit(hdr.mac_sa);
        packet.emit(hdr.len_or_type);
        packet.emit(hdr.pcp);
        packet.emit(hdr.cfi);
        packet.emit(hdr.vlan_id);
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
