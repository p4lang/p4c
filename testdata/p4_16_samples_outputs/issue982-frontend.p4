#include <core.p4>

header clone_0_t {
    bit<16> data;
}

header clone_1_t {
    bit<32> data;
}

header_union clone_union_t {
    clone_0_t h0;
    clone_1_t h1;
}

struct clone_metadata_t {
    bit<3>        type;
    clone_union_t data;
}

typedef clone_metadata_t CloneMetadata_t;
typedef bit<10> PortId_t;
typedef bit<10> MulticastGroup_t;
typedef bit<3> ClassOfService_t;
typedef bit<14> PacketLength_t;
typedef bit<16> EgressInstance_t;
typedef bit<48> Timestamp_t;
typedef error ParserError_t;
enum InstanceType_t {
    NORMAL,
    CLONE,
    RESUBMIT,
    RECIRCULATE
}

struct psa_ingress_parser_input_metadata_t {
    PortId_t       ingress_port;
    InstanceType_t instance_type;
}

struct psa_egress_parser_input_metadata_t {
    PortId_t        egress_port;
    InstanceType_t  instance_type;
    CloneMetadata_t clone_metadata;
}

struct psa_parser_output_metadata_t {
    ParserError_t parser_error;
}

struct psa_ingress_deparser_output_metadata_t {
    CloneMetadata_t clone_metadata;
}

struct psa_egress_deparser_output_metadata_t {
    CloneMetadata_t clone_metadata;
}

struct psa_ingress_input_metadata_t {
    PortId_t       ingress_port;
    InstanceType_t instance_type;
    Timestamp_t    ingress_timestamp;
    ParserError_t  parser_error;
}

struct psa_ingress_output_metadata_t {
    ClassOfService_t class_of_service;
    bool             clone;
    PortId_t         clone_port;
    ClassOfService_t clone_class_of_service;
    bool             drop;
    bool             resubmit;
    MulticastGroup_t multicast_group;
    PortId_t         egress_port;
    bool             truncate;
    PacketLength_t   truncate_payload_bytes;
}

struct psa_egress_input_metadata_t {
    ClassOfService_t class_of_service;
    PortId_t         egress_port;
    InstanceType_t   instance_type;
    EgressInstance_t instance;
    Timestamp_t      egress_timestamp;
    ParserError_t    parser_error;
}

struct psa_egress_output_metadata_t {
    bool             clone;
    ClassOfService_t clone_class_of_service;
    bool             drop;
    bool             recirculate;
    bool             truncate;
    PacketLength_t   truncate_payload_bytes;
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
    Digest(PortId_t receiver);
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
typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    fwd_metadata_t fwd_metadata;
    bit<3>         custom_clone_id;
    clone_0_t      clone_0;
    clone_1_t      clone_1;
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
        }
    }
    state parse_ethernet {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        transition CommonParser_start;
    }
    state CommonParser_start {
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
        transition CloneParser_start;
    }
    state CloneParser_start {
        transition select(istd.clone_metadata.type) {
            3w0: CloneParser_parse_clone_header;
            3w1: CloneParser_parse_clone_header_0;
            default: reject;
        }
    }
    state CloneParser_parse_clone_header {
        user_meta.custom_clone_id = istd.clone_metadata.type;
        user_meta.clone_0 = istd.clone_metadata.data.h0;
        transition parse_clone_header_2;
    }
    state CloneParser_parse_clone_header_0 {
        user_meta.custom_clone_id = istd.clone_metadata.type;
        user_meta.clone_1 = istd.clone_metadata.data.h1;
        transition parse_clone_header_2;
    }
    state parse_clone_header_2 {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("egress.process_clone_h0") action process_clone_h0() {
        user_meta.fwd_metadata.outport = (bit<32>)user_meta.clone_0.data;
    }
    @name("egress.process_clone_h1") action process_clone_h1() {
        user_meta.fwd_metadata.outport = user_meta.clone_1.data;
    }
    @name("egress.t") table t_0 {
        key = {
            user_meta.custom_clone_id: exact @name("user_meta.custom_clone_id") ;
        }
        actions = {
            process_clone_h0();
            process_clone_h1();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t_0.apply();
    }
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, out psa_parser_output_metadata_t ostd) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        parsed_hdr.ipv4.setInvalid();
        transition CommonParser_start_0;
    }
    state CommonParser_start_0 {
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
    @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_clone") action do_clone(PortId_t port) {
        ostd.clone = true;
        ostd.clone_port = port;
        user_meta.custom_clone_id = 3w1;
    }
    @name("ingress.t") table t_1 {
        key = {
            user_meta.fwd_metadata.outport: exact @name("user_meta.fwd_metadata.outport") ;
        }
        actions = {
            do_clone();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_1.apply();
    }
}

control IngressDeparserImpl(packet_out packet, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd, out psa_ingress_deparser_output_metadata_t ostd) {
    clone_metadata_t clone_md_0;
    apply {
        clone_md_0.data.h1.setValid();
        clone_md_0.data.h1 = clone_1_t {data = 32w0};
        clone_md_0.type = 3w0;
        if (meta.custom_clone_id == 3w1) {
            ostd.clone_metadata = clone_md_0;
        }
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out packet, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, out psa_egress_deparser_output_metadata_t ostd) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

PSA_Switch<headers, metadata, headers, metadata>(IngressParserImpl(), ingress(), IngressDeparserImpl(), EgressParserImpl(), egress(), EgressDeparserImpl()) main;

