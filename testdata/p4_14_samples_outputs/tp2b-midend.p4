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
header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> b1;
    bit<32> b2;
    bit<32> b3;
    bit<32> b4;
}

struct metadata {
}

struct headers {
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_2() {
    }
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    @name("setf1") action setf1(bit<32> val) {
        hdr.data.f1 = val;
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_2() {
    }
    @name("noop") action noop_3() {
    }
    @name("setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name("setb2") action setb2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name("E1") table E1_0() {
        actions = {
            setf1();
            noop();
            NoAction_2();
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_2();
    }
    @name("EA") table EA_0() {
        actions = {
            setb1();
            noop_2();
            NoAction_3();
        }
        key = {
            hdr.data.f3: ternary;
        }
        default_action = NoAction_3();
    }
    @name("EB") table EB_0() {
        actions = {
            setb2();
            noop_3();
            NoAction_4();
        }
        key = {
            hdr.data.f3: ternary;
        }
        default_action = NoAction_4();
    }
    apply {
        E1_0.apply();
        if (hdr.data.f1 == 32w0) 
            EA_0.apply();
        else 
            EB_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    action NoAction_7() {
    }
    action NoAction_8() {
    }
    @name("setb1") action setb1_2(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name("noop") action noop_4() {
    }
    @name("noop") action noop_5() {
    }
    @name("noop") action noop_6() {
    }
    @name("noop") action noop_7() {
    }
    @name("setb3") action setb3(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name("setb2") action setb2_2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name("setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name("A1") table A1_0() {
        actions = {
            setb1_2();
            noop_4();
            NoAction_5();
        }
        key = {
            hdr.data.f1: ternary;
        }
        default_action = NoAction_5();
    }
    @name("A2") table A2_0() {
        actions = {
            setb3();
            noop_5();
            NoAction_6();
        }
        key = {
            hdr.data.b1: ternary;
        }
        default_action = NoAction_6();
    }
    @name("B1") table B1_0() {
        actions = {
            setb2_2();
            noop_6();
            NoAction_7();
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_7();
    }
    @name("B2") table B2_0() {
        actions = {
            setb4();
            noop_7();
            NoAction_8();
        }
        key = {
            hdr.data.b2: ternary;
        }
        default_action = NoAction_8();
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1_0.apply();
            A2_0.apply();
        }
        B1_0.apply();
        B2_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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
