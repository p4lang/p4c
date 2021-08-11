#include <core.p4>
#include <dpdk/psa.p4>

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
    bit<32> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct empty_metadata_t {
}

struct metadata_t {
    bit<32> port_in;
    bit<32> port_out;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser IngressParserImpl(packet_in buffer, out headers hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        buffer.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract(hdr.ipv4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    Counter<bit<10>, bit<12>>(1024, PSA_CounterType_t.PACKETS_AND_BYTES) counter0;
    Counter<bit<10>, bit<12>>(1024, PSA_CounterType_t.PACKETS) counter1;
    Counter<bit<10>, bit<12>>(1024, PSA_CounterType_t.BYTES) counter2;
    Register<bit<32>, bit<12>>(1024) reg;
    Meter<bit<12>>(1024, PSA_MeterType_t.BYTES) meter0;
    PSA_MeterColor_t color_out;
    PSA_MeterColor_t color_in = PSA_MeterColor_t.RED;
    action execute(bit<12> index) {
        color_out = meter0.execute(index, color_in, hdr.ipv4.totalLen);
        user_meta.port_out = (color_out == PSA_MeterColor_t.GREEN ? 32w1 : 32w0);
        reg.write(index, user_meta.port_out);
    }
    table tbl {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            NoAction;
            execute;
        }
    }
    apply {
        if (user_meta.port_out == 1) {
            tbl.apply();
            counter0.count(1023, 20);
            counter1.count(512);
            counter2.count(1023, 64);
            user_meta.port_out = reg.read(1);
        } else {
            return;
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

