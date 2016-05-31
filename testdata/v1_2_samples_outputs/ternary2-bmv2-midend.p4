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
header data_h {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

header extra_h {
    bit<16> h;
    bit<8>  b1;
    bit<8>  b2;
}

struct packet_t {
    data_h     data;
    extra_h[4] extra;
}

struct Meta {
}

parser p(packet_in b, out packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    state start {
        b.extract(hdrs.data);
        transition extra;
    }
    state extra {
        b.extract(hdrs.extra.next);
        transition select(hdrs.extra.last.b2) {
            8w0x80 &&& 8w0x80: extra;
            default: accept;
        }
    }
}

control vrfy(in packet_t h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control update(inout packet_t h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control ingress(inout packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    bit<8> reg_0;
    bit<8> reg_1;
    bit<8> reg_2;
    bit<8> reg_3;
    @name("setb1") action setb1(bit<9> port, bit<8> val) {
        hdrs.data.b1 = val;
        meta.egress_spec = port;
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_1() {
    }
    @name("noop") action noop_2() {
    }
    @name("noop") action noop_3() {
    }
    @name("noop") action noop_4() {
    }
    @name("setbyte") action setbyte(bit<8> val) {
        reg_0 = val;
        hdrs.extra[0].b1 = reg_0;
    }
    @name("setbyte") action setbyte_1(bit<8> val) {
        reg_1 = val;
        hdrs.data.b2 = reg_1;
    }
    @name("setbyte") action setbyte_2(bit<8> val) {
        reg_2 = val;
        hdrs.extra[1].b1 = reg_2;
    }
    @name("setbyte") action setbyte_3(bit<8> val) {
        reg_3 = val;
        hdrs.extra[2].b2 = reg_3;
    }
    @name("act1") action act1(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("act2") action act2(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("act3") action act3(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("test1") table test1_0() {
        key = {
            hdrs.data.f1: ternary;
        }
        actions = {
            setb1();
            noop();
        }
        default_action = noop();
    }
    @name("ex1") table ex1_0() {
        key = {
            hdrs.extra[0].h: ternary;
        }
        actions = {
            setbyte();
            act1();
            act2();
            act3();
            noop_1();
        }
        default_action = noop_1();
    }
    @name("tbl1") table tbl1_0() {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte_1();
            noop_2();
        }
        default_action = noop_2();
    }
    @name("tbl2") table tbl2_0() {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte_2();
            noop_3();
        }
        default_action = noop_3();
    }
    @name("tbl3") table tbl3_0() {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte_3();
            noop_4();
        }
        default_action = noop_4();
    }
    apply {
        test1_0.apply();
        switch (ex1_0.apply().action_run) {
            act1: {
                tbl1_0.apply();
            }
            act2: {
                tbl2_0.apply();
            }
            act3: {
                tbl3_0.apply();
            }
        }

    }
}

control egress(inout packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    apply {
    }
}

control deparser(packet_out b, in packet_t hdrs) {
    apply {
        b.emit(hdrs.data);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
