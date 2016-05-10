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

extern void mark_to_drop();
extern ActionSelector {
    ActionSelector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(in H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeCkecksum<H, M>(inout H hdr, inout M meta);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg, ComputeCkecksum<H, M> ck, Deparser<H> dep);
@name("queueing_metadata_t") struct queueing_metadata_t_0 {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

header queueing_metadata_t {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

struct metadata {
    @name("queueing_metadata") 
    queueing_metadata_t_0 queueing_metadata;
}

struct headers {
    @name("hdr1") 
    hdr1_t              hdr1;
    @name("queueing_hdr") 
    queueing_metadata_t queueing_hdr;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("queueing_dummy") state queueing_dummy {
        packet.extract(hdr.queueing_hdr);
        transition accept;
    }
    @name("start") state start {
        packet.extract(hdr.hdr1);
        transition select(standard_metadata.packet_length) {
            32w0: queueing_dummy;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("copy_queueing_data") action copy_queueing_data() {
        hdr.queueing_hdr.setValid();
        hdr.queueing_hdr.enq_timestamp = meta.queueing_metadata.enq_timestamp;
        hdr.queueing_hdr.enq_qdepth = meta.queueing_metadata.enq_qdepth;
        hdr.queueing_hdr.deq_timedelta = meta.queueing_metadata.deq_timedelta;
        hdr.queueing_hdr.deq_qdepth = meta.queueing_metadata.deq_qdepth;
    }
    @name("t_egress") table t_egress() {
        actions = {
            copy_queueing_data;
            NoAction;
        }
        default_action = NoAction();
    }
    apply {
        t_egress.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("set_port") action set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("t_ingress") table t_ingress() {
        actions = {
            set_port;
            _drop;
            NoAction;
        }
        key = {
            hdr.hdr1.f1: exact;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.hdr1);
        packet.emit(hdr.queueing_hdr);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
