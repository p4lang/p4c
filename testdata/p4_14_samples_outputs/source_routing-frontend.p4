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

action NoAction() {
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
extern action_selector {
    action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(in H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeChecksum<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg, ComputeChecksum<H, M> ck, Deparser<H> dep);
header easyroute_head_t {
    bit<64> preamble;
    bit<32> num_valid;
}

header easyroute_port_t {
    bit<8> port;
}

struct metadata {
}

struct headers {
    @name("easyroute_head") 
    easyroute_head_t easyroute_head;
    @name("easyroute_port") 
    easyroute_port_t easyroute_port;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_head") state parse_head {
        packet.extract<easyroute_head_t>(hdr.easyroute_head);
        transition select(hdr.easyroute_head.num_valid) {
            32w0: accept;
            default: parse_port;
        }
    }
    @name("parse_port") state parse_port {
        packet.extract<easyroute_port_t>(hdr.easyroute_port);
        transition accept;
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
        standard_metadata.egress_spec = (bit<9>)hdr.easyroute_port.port;
        hdr.easyroute_head.num_valid = hdr.easyroute_head.num_valid + 32w4294967295;
        hdr.easyroute_port.setInvalid();
    }
    @name("route_pkt") table route_pkt() {
        actions = {
            _drop();
            route();
            NoAction();
        }
        key = {
            hdr.easyroute_port.isValid(): exact;
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        route_pkt.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<easyroute_head_t>(hdr.easyroute_head);
        packet.emit<easyroute_port_t>(hdr.easyroute_port);
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
