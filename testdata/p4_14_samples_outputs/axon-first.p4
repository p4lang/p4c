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
struct my_metadata_t {
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
    bit<16> headerLen;
}

header axon_head_t {
    bit<64> preamble;
    bit<8>  axonType;
    bit<16> axonLength;
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
}

header axon_hop_t {
    bit<8> port;
}

struct metadata {
    @name("my_metadata") 
    my_metadata_t my_metadata;
}

struct headers {
    @name("axon_head") 
    axon_head_t    axon_head;
    @name("axon_fwdHop") 
    axon_hop_t[64] axon_fwdHop;
    @name("axon_revHop") 
    axon_hop_t[64] axon_revHop;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_fwdHop") state parse_fwdHop {
        packet.extract<axon_hop_t>(hdr.axon_fwdHop.next);
        meta.my_metadata.fwdHopCount = meta.my_metadata.fwdHopCount + 8w255;
        transition parse_next_fwdHop;
    }
    @name("parse_head") state parse_head {
        packet.extract<axon_head_t>(hdr.axon_head);
        meta.my_metadata.fwdHopCount = hdr.axon_head.fwdHopCount;
        meta.my_metadata.revHopCount = hdr.axon_head.revHopCount;
        meta.my_metadata.headerLen = (bit<16>)(8w2 + hdr.axon_head.fwdHopCount + hdr.axon_head.revHopCount);
        transition select(hdr.axon_head.fwdHopCount) {
            8w0: accept;
            default: parse_next_fwdHop;
        }
    }
    @name("parse_next_fwdHop") state parse_next_fwdHop {
        transition select(meta.my_metadata.fwdHopCount) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop;
        }
    }
    @name("parse_next_revHop") state parse_next_revHop {
        transition select(meta.my_metadata.revHopCount) {
            8w0x0: accept;
            default: parse_revHop;
        }
    }
    @name("parse_revHop") state parse_revHop {
        packet.extract<axon_hop_t>(hdr.axon_revHop.next);
        meta.my_metadata.revHopCount = meta.my_metadata.revHopCount + 8w255;
        transition parse_next_revHop;
    }
    @name("start") state start {
        transition select((packet.lookahead<bit<64>>())[63:0]) {
            64w0: parse_head;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("route") action route() {
        standard_metadata.egress_spec = (bit<9>)hdr.axon_fwdHop[0].port;
        hdr.axon_head.fwdHopCount = hdr.axon_head.fwdHopCount + 8w255;
        hdr.axon_fwdHop.pop_front(1);
        hdr.axon_head.revHopCount = hdr.axon_head.revHopCount + 8w1;
        hdr.axon_revHop.push_front(1);
        hdr.axon_revHop[0].port = (bit<8>)standard_metadata.ingress_port;
    }
    @name("drop_pkt") table drop_pkt() {
        actions = {
            _drop();
            NoAction();
        }
        size = 1;
        default_action = NoAction();
    }
    @name("route_pkt") table route_pkt() {
        actions = {
            _drop();
            route();
            NoAction();
        }
        key = {
            hdr.axon_head.isValid()     : exact;
            hdr.axon_fwdHop[0].isValid(): exact;
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        if (hdr.axon_head.axonLength != meta.my_metadata.headerLen) 
            drop_pkt.apply();
        else 
            route_pkt.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<axon_head_t>(hdr.axon_head);
        packet.emit<axon_hop_t[64]>(hdr.axon_fwdHop);
        packet.emit<axon_hop_t[64]>(hdr.axon_revHop);
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
