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
    @name("drop") action drop() {
    }
    @name("forward") action forward(bit<48> to) {
        hdr.ethernet.dstAddr = to;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("do_setup") action do_setup() {
    }
    @name("route") table route() {
        actions = {
            drop();
            forward();
            NoAction();
        }
        key = {
            hdr.ipv4.dstAddr: ternary;
        }
        default_action = NoAction();
    }
    @name("setup") table setup() {
        actions = {
            do_setup();
            NoAction();
        }
        key = {
            hdr.ethernet.isValid(): exact;
        }
        default_action = NoAction();
    }
    apply {
        setup.apply();
        route.apply();
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

struct struct_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Checksum16() ipv4_checksum;
    apply {
        if (hdr.ipv4.hdrChecksum == ipv4_checksum.get<struct_0>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr })) 
            mark_to_drop();
    }
}

struct struct_1 {
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

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Checksum16() ipv4_checksum;
    apply {
        hdr.ipv4.hdrChecksum = ipv4_checksum.get<struct_1>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
