#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_base_t {
    bit<8>  version_ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> flags_fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header ipv4_option_timestamp_t {
    bit<8>      value;
    bit<8>      len;
    varbit<304> data;
}

struct headers_t {
    ethernet_t              ethernet;
    ipv4_base_t             ipv4_base;
    ipv4_option_timestamp_t ipv4_option_timestamp;
}

struct EMPTY {
}

parser MyIP(packet_in packet, out headers_t hdr, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    @name("MyIP.tmp16") bit<16> tmp16_0;
    @name("MyIP.tmp_0") bit<8> tmp_0;
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_base_t>(hdr.ipv4_base);
        transition select(hdr.ipv4_base.version_ihl) {
            8w0x45: accept;
            default: parse_ipv4_options;
        }
    }
    state parse_ipv4_option_timestamp {
        tmp16_0 = packet.lookahead<bit<16>>();
        packet.extract<ipv4_option_timestamp_t>(hdr.ipv4_option_timestamp, ((bit<32>)tmp16_0[7:0] << 3) + 32w4294967280);
        transition accept;
    }
    state parse_ipv4_options {
        tmp_0 = packet.lookahead<bit<8>>();
        transition select(tmp_0) {
            8w0x44: parse_ipv4_option_timestamp;
            default: accept;
        }
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        d.drop = false;
        d.multicast_group = 32w0;
        d.egress_port = 32w0;
    }
    @name("MyIC.ap") ActionProfile(32w1024) ap_0;
    @name("MyIC.a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name("MyIC.a2") action a2(@name("param") bit<16> param_2) {
        hdr.ethernet.etherType = param_2;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            a2();
        }
        psa_implementation = ap_0;
        default_action = NoAction_1();
    }
    @name("MyIC.tbl2") table tbl2_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_2();
            a1();
        }
        psa_implementation = ap_0;
        default_action = NoAction_2();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        tbl_send_to_port.apply();
        tbl_0.apply();
        tbl2_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in EMPTY e, in psa_ingress_output_metadata_t f) {
    @hidden action psaexampledpdkvarbitbmv2l135() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_base_t>(hdr.ipv4_base);
    }
    @hidden table tbl_psaexampledpdkvarbitbmv2l135 {
        actions = {
            psaexampledpdkvarbitbmv2l135();
        }
        const default_action = psaexampledpdkvarbitbmv2l135();
    }
    apply {
        tbl_psaexampledpdkvarbitbmv2l135.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
