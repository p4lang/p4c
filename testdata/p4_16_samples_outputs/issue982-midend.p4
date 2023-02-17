#include <core.p4>

header clone_0_t {
    bit<16> data;
}

header clone_1_t {
    bit<32> data;
}

struct clone_metadata_t {
    bit<3>    type;
    clone_0_t data_h0;
    clone_1_t data_h1;
}

enum InstanceType_t {
    NORMAL,
    CLONE,
    RESUBMIT,
    RECIRCULATE
}

struct psa_ingress_parser_input_metadata_t {
    bit<10>        ingress_port;
    InstanceType_t instance_type;
}

struct psa_egress_parser_input_metadata_t {
    bit<10>          egress_port;
    InstanceType_t   instance_type;
    clone_metadata_t clone_metadata;
}

struct psa_parser_output_metadata_t {
    error parser_error;
}

struct psa_ingress_deparser_output_metadata_t {
    clone_metadata_t clone_metadata;
}

struct psa_egress_deparser_output_metadata_t {
    clone_metadata_t clone_metadata;
}

struct psa_ingress_input_metadata_t {
    bit<10>        ingress_port;
    InstanceType_t instance_type;
    bit<48>        ingress_timestamp;
    error          parser_error;
}

struct psa_ingress_output_metadata_t {
    bit<3>  class_of_service;
    bool    clone;
    bit<10> clone_port;
    bit<3>  clone_class_of_service;
    bool    drop;
    bool    resubmit;
    bit<10> multicast_group;
    bit<10> egress_port;
    bool    truncate;
    bit<14> truncate_payload_bytes;
}

struct psa_egress_input_metadata_t {
    bit<3>         class_of_service;
    bit<10>        egress_port;
    InstanceType_t instance_type;
    bit<16>        instance;
    bit<48>        egress_timestamp;
    error          parser_error;
}

struct psa_egress_output_metadata_t {
    bool    clone;
    bit<3>  clone_class_of_service;
    bool    drop;
    bool    recirculate;
    bool    truncate;
    bit<14> truncate_payload_bytes;
}

match_kind {
    range,
    selector
}

extern PacketReplicationEngine {
}

extern BufferingQueueingEngine {
}

extern clone {
    void emit<T>(in T hdr);
}

extern resubmit {
    void emit<T>(in T hdr);
}

extern recirculate {
    void emit<T>(in T hdr);
}

enum HashAlgorithm_t {
    IDENTITY,
    CRC32,
    CRC32_CUSTOM,
    CRC16,
    CRC16_CUSTOM,
    ONES_COMPLEMENT16,
    TARGET_DEFAULT
}

extern Hash<O> {
    Hash(HashAlgorithm_t algo);
    O get_hash<D>(in D data);
    O get_hash<T, D>(in T base, in D data, in T max);
}

extern Checksum<W> {
    Checksum(HashAlgorithm_t hash);
    void clear();
    void update<T>(in T data);
    W get();
}

extern InternetChecksum {
    InternetChecksum();
    void clear();
    void update<T>(in T data);
    void remove<T>(in T data);
    bit<16> get();
}

enum CounterType_t {
    PACKETS,
    BYTES,
    PACKETS_AND_BYTES
}

extern Counter<W, S> {
    Counter(bit<32> n_counters, CounterType_t type);
    void count(in S index);
}

extern DirectCounter<W> {
    DirectCounter(CounterType_t type);
    void count();
}

enum MeterType_t {
    PACKETS,
    BYTES
}

enum MeterColor_t {
    RED,
    GREEN,
    YELLOW
}

extern Meter<S> {
    Meter(bit<32> n_meters, MeterType_t type);
    MeterColor_t execute(in S index, in MeterColor_t color);
    MeterColor_t execute(in S index);
}

extern DirectMeter {
    DirectMeter(MeterType_t type);
    MeterColor_t execute(in MeterColor_t color);
    MeterColor_t execute();
}

extern Register<T, S> {
    Register(bit<32> size);
    T read(in S index);
    void write(in S index, in T value);
}

extern Random<T> {
    Random(T min, T max);
    T read();
}

extern ActionProfile {
    ActionProfile(bit<32> size);
}

extern ActionSelector {
    ActionSelector(HashAlgorithm_t algo, bit<32> size, bit<32> outputWidth);
}

extern Digest<T> {
    Digest(bit<10> receiver);
    void emit(in T data);
}

extern ValueSet<D> {
    ValueSet(int<32> size);
    bool is_member(in D data);
}

parser IngressParser<H, M>(packet_in buffer, out H parsed_hdr, inout M user_meta, in psa_ingress_parser_input_metadata_t istd, out psa_parser_output_metadata_t ostd);
control Ingress<H, M>(inout H hdr, inout M user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd);
parser EgressParser<H, M>(packet_in buffer, out H parsed_hdr, inout M user_meta, in psa_egress_parser_input_metadata_t istd, out psa_parser_output_metadata_t ostd);
control Egress<H, M>(inout H hdr, inout M user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd);
control IngressDeparser<H, M>(packet_out buffer, inout H hdr, in M user_meta, in psa_ingress_output_metadata_t istd, out psa_ingress_deparser_output_metadata_t ostd);
control EgressDeparser<H, M>(packet_out buffer, inout H hdr, in M user_meta, in psa_egress_output_metadata_t istd, out psa_egress_deparser_output_metadata_t ostd);
package PSA_Switch<IH, IM, EH, EM>(IngressParser<IH, IM> ip, Ingress<IH, IM> ig, IngressDeparser<IH, IM> id, EgressParser<EH, EM> ep, Egress<EH, EM> eg, EgressDeparser<EH, EM> ed);
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

struct fwd_metadata_t {
    bit<32> outport;
}

struct metadata {
    bit<32>   _fwd_metadata_outport0;
    bit<3>    _custom_clone_id1;
    clone_0_t _clone_02;
    clone_1_t _clone_13;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, out psa_parser_output_metadata_t ostd) {
    state start {
        transition select(istd.instance_type) {
            InstanceType_t.CLONE: parse_clone_header;
            InstanceType_t.NORMAL: parse_ethernet;
            default: noMatch;
        }
    }
    state parse_ethernet {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: parse_ethernet_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition parse_ethernet_0;
    }
    state parse_ethernet_0 {
        transition accept;
    }
    state parse_clone_header {
        transition select(istd.clone_metadata.type) {
            3w0: CloneParser_parse_clone_header;
            3w1: CloneParser_parse_clone_header_0;
            default: reject;
        }
    }
    state CloneParser_parse_clone_header {
        user_meta._custom_clone_id1 = istd.clone_metadata.type;
        user_meta._clone_02 = istd.clone_metadata.data_h0;
        transition parse_clone_header_2;
    }
    state CloneParser_parse_clone_header_0 {
        user_meta._custom_clone_id1 = istd.clone_metadata.type;
        user_meta._clone_13 = istd.clone_metadata.data_h1;
        transition parse_clone_header_2;
    }
    state parse_clone_header_2 {
        transition parse_ethernet;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("egress.process_clone_h0") action process_clone_h0() {
        user_meta._fwd_metadata_outport0 = (bit<32>)user_meta._clone_02.data;
    }
    @name("egress.process_clone_h1") action process_clone_h1() {
        user_meta._fwd_metadata_outport0 = user_meta._clone_13.data;
    }
    @name("egress.t") table t_0 {
        key = {
            user_meta._custom_clone_id1: exact @name("user_meta.custom_clone_id");
        }
        actions = {
            process_clone_h0();
            process_clone_h1();
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        t_0.apply();
    }
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, out psa_parser_output_metadata_t ostd) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr.ipv4);
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.do_clone") action do_clone(@name("port") bit<10> port) {
        ostd.clone = true;
        ostd.clone_port = port;
        user_meta._custom_clone_id1 = 3w1;
    }
    @name("ingress.t") table t_1 {
        key = {
            user_meta._fwd_metadata_outport0: exact @name("user_meta.fwd_metadata.outport");
        }
        actions = {
            do_clone();
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        t_1.apply();
    }
}

control IngressDeparserImpl(packet_out packet, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd, out psa_ingress_deparser_output_metadata_t ostd) {
    clone_0_t clone_md_0_data_h0;
    clone_1_t clone_md_0_data_h1;
    @hidden action issue982l420() {
        ostd.clone_metadata.type = 3w0;
        if (clone_md_0_data_h0.isValid()) {
            ostd.clone_metadata.data_h0.setValid();
            ostd.clone_metadata.data_h0 = clone_md_0_data_h0;
            ostd.clone_metadata.data_h1.setInvalid();
        } else {
            ostd.clone_metadata.data_h0.setInvalid();
        }
        if (clone_md_0_data_h1.isValid()) {
            ostd.clone_metadata.data_h1.setValid();
            ostd.clone_metadata.data_h1 = clone_md_0_data_h1;
            ostd.clone_metadata.data_h0.setInvalid();
        } else {
            ostd.clone_metadata.data_h1.setInvalid();
        }
    }
    @hidden action issue982l416() {
        clone_md_0_data_h0.setInvalid();
        clone_md_0_data_h1.setInvalid();
        clone_md_0_data_h1.setValid();
        clone_md_0_data_h0.setInvalid();
        clone_md_0_data_h1.setValid();
        clone_md_0_data_h1.data = 32w0;
        clone_md_0_data_h0.setInvalid();
    }
    @hidden action issue982l422() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_issue982l416 {
        actions = {
            issue982l416();
        }
        const default_action = issue982l416();
    }
    @hidden table tbl_issue982l420 {
        actions = {
            issue982l420();
        }
        const default_action = issue982l420();
    }
    @hidden table tbl_issue982l422 {
        actions = {
            issue982l422();
        }
        const default_action = issue982l422();
    }
    apply {
        tbl_issue982l416.apply();
        if (meta._custom_clone_id1 == 3w1) {
            tbl_issue982l420.apply();
        }
        tbl_issue982l422.apply();
    }
}

control EgressDeparserImpl(packet_out packet, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, out psa_egress_deparser_output_metadata_t ostd) {
    @hidden action issue982l429() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_issue982l429 {
        actions = {
            issue982l429();
        }
        const default_action = issue982l429();
    }
    apply {
        tbl_issue982l429.apply();
    }
}

PSA_Switch<headers, metadata, headers, metadata>(IngressParserImpl(), ingress(), IngressDeparserImpl(), EgressParserImpl(), egress(), EgressDeparserImpl()) main;
