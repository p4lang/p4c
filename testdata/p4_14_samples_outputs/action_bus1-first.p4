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
header data_t {
    bit<32> f1_1;
    bit<32> f1_2;
    bit<32> f1_3;
    bit<32> f1_4;
    bit<32> f1_5;
    bit<32> f2_1;
    bit<32> f2_2;
    bit<32> f2_3;
    bit<32> f2_4;
    bit<32> f2_5;
    bit<32> f3_1;
    bit<32> f3_2;
    bit<32> f3_3;
    bit<32> f3_4;
    bit<32> f3_5;
    bit<32> f4_1;
    bit<32> f4_2;
    bit<32> f4_3;
    bit<32> f4_4;
    bit<32> f4_5;
    bit<32> f5_1;
    bit<32> f5_2;
    bit<32> f5_3;
    bit<32> f5_4;
    bit<32> f5_5;
    bit<32> f6_1;
    bit<32> f6_2;
    bit<32> f6_3;
    bit<32> f6_4;
    bit<32> f6_5;
    bit<32> f7_1;
    bit<32> f7_2;
    bit<32> f7_3;
    bit<32> f7_4;
    bit<32> f7_5;
    bit<32> f8_1;
    bit<32> f8_2;
    bit<32> f8_3;
    bit<32> f8_4;
    bit<32> f8_5;
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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("set1") action set1(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f1_1 = v1;
        hdr.data.f1_2 = v2;
        hdr.data.f1_3 = v3;
        hdr.data.f1_4 = v4;
        hdr.data.f1_5 = v5;
    }
    @name("noop") action noop() {
    }
    @name("set2") action set2(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f2_1 = v1;
        hdr.data.f2_2 = v2;
        hdr.data.f2_3 = v3;
        hdr.data.f2_4 = v4;
        hdr.data.f2_5 = v5;
    }
    @name("set3") action set3(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f3_1 = v1;
        hdr.data.f3_2 = v2;
        hdr.data.f3_3 = v3;
        hdr.data.f3_4 = v4;
        hdr.data.f3_5 = v5;
    }
    @name("set4") action set4(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f4_1 = v1;
        hdr.data.f4_2 = v2;
        hdr.data.f4_3 = v3;
        hdr.data.f4_4 = v4;
        hdr.data.f4_5 = v5;
    }
    @name("set5") action set5(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f5_1 = v1;
        hdr.data.f5_2 = v2;
        hdr.data.f5_3 = v3;
        hdr.data.f5_4 = v4;
        hdr.data.f5_5 = v5;
    }
    @name("set6") action set6(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f6_1 = v1;
        hdr.data.f6_2 = v2;
        hdr.data.f6_3 = v3;
        hdr.data.f6_4 = v4;
        hdr.data.f6_5 = v5;
    }
    @name("set7") action set7(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f7_1 = v1;
        hdr.data.f7_2 = v2;
        hdr.data.f7_3 = v3;
        hdr.data.f7_4 = v4;
        hdr.data.f7_5 = v5;
    }
    @name("set8") action set8(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f8_1 = v1;
        hdr.data.f8_2 = v2;
        hdr.data.f8_3 = v3;
        hdr.data.f8_4 = v4;
        hdr.data.f8_5 = v5;
    }
    @name("tbl1") table tbl1() {
        actions = {
            set1();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f1_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl2") table tbl2() {
        actions = {
            set2();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f2_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl3") table tbl3() {
        actions = {
            set3();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f3_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl4") table tbl4() {
        actions = {
            set4();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f4_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl5") table tbl5() {
        actions = {
            set5();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f5_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl6") table tbl6() {
        actions = {
            set6();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f6_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl7") table tbl7() {
        actions = {
            set7();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f7_1: exact;
        }
        default_action = NoAction();
    }
    @name("tbl8") table tbl8() {
        actions = {
            set8();
            noop();
            NoAction();
        }
        key = {
            hdr.data.f8_1: exact;
        }
        default_action = NoAction();
    }
    apply {
        tbl1.apply();
        tbl2.apply();
        tbl3.apply();
        tbl4.apply();
        tbl5.apply();
        tbl6.apply();
        tbl7.apply();
        tbl8.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
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
