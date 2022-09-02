error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}
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
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.version) {
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.PACKETS_AND_BYTES) counter0;
    Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.PACKETS) counter1;
    Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.BYTES) counter2;
    Register<bit<32>, bit<12>>(32w1024) reg;
    Meter<bit<12>>(32w1024, PSA_MeterType_t.BYTES) meter0;
    PSA_MeterColor_t color_out;
    PSA_MeterColor_t color_in = PSA_MeterColor_t.RED;
    action execute(bit<12> index) {
        hdr.ipv4.ihl = 4w5;
        color_out = meter0.execute(index, color_in, hdr.ipv4.totalLen);
        user_meta.port_out = (color_out == PSA_MeterColor_t.GREEN ? 32w1 : 32w0);
        reg.write(index, user_meta.port_out);
        if (hdr.ipv4.hdrChecksum[5:0] == 6w6) {
            hdr.ipv4.ihl = 4w5;
        }
        if (hdr.ipv4.version == 4w6) {
            hdr.ipv4.ihl = 4w6;
        }
    }
    action test(bit<4> version) {
        if (version == 4w4) {
            hdr.ipv4.hdrChecksum[3:0] = hdr.ipv4.version + 4w5;
        }
    }
    table tbl {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction();
            execute();
        }
        default_action = NoAction();
    }
    apply {
        if (user_meta.port_out == 32w1) {
            tbl.apply();
            counter0.count(12w1023, 32w20);
            counter1.count(12w512);
            counter2.count(12w1023, 32w64);
            user_meta.port_out = reg.read(12w1);
            test(hdr.ipv4.version);
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
        hdr.ipv4.hdrChecksum[3:0] = 4w4;
        hdr.ipv4.version = 4w4;
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata_t, headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

