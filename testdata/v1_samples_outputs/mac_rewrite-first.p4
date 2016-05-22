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
extern ActionSelector {
    ActionSelector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
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
struct egress_metadata_t {
    bit<16> payload_length;
    bit<9>  smac_idx;
    bit<16> bd;
    bit<1>  inner_replica;
    bit<1>  replica;
    bit<48> mac_da;
    bit<1>  routed;
    bit<16> same_bd_check;
    bit<4>  header_count;
    bit<8>  drop_reason;
    bit<1>  egress_bypass;
    bit<8>  drop_exception;
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
    @name("egress_metadata") 
    egress_metadata_t egress_metadata;
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
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
    @name("parse_ipv6") state parse_ipv6 {
        packet.extract(hdr.ipv6);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control process_mac_rewrite(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("nop") action nop() {
    }
    @name("rewrite_ipv4_unicast_mac") action rewrite_ipv4_unicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("rewrite_ipv4_multicast_mac") action rewrite_ipv4_multicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:23] = 25w0x0;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("rewrite_ipv6_unicast_mac") action rewrite_ipv6_unicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name("rewrite_ipv6_multicast_mac") action rewrite_ipv6_multicast_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:32] = 16w0x0;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name("mac_rewrite") table mac_rewrite() {
        actions = {
            nop();
            rewrite_ipv4_unicast_mac();
            rewrite_ipv4_multicast_mac();
            rewrite_ipv6_unicast_mac();
            rewrite_ipv6_multicast_mac();
            NoAction();
        }
        key = {
            meta.egress_metadata.smac_idx: exact;
            hdr.ipv4.isValid()           : exact;
            hdr.ipv6.isValid()           : exact;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (meta.egress_metadata.routed == 1w1) 
            mac_rewrite.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("do_setup") action do_setup(bit<9> idx, bit<1> routed) {
        meta.egress_metadata.mac_da = hdr.ethernet.dstAddr;
        meta.egress_metadata.smac_idx = idx;
        meta.egress_metadata.routed = routed;
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
    process_mac_rewrite() @name("process_mac_rewrite") process_mac_rewrite_0;
    apply {
        setup.apply();
        process_mac_rewrite_0.apply(hdr, meta, standard_metadata);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.ipv4);
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
