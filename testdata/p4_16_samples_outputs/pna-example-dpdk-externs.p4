#include <core.p4>
#include <pna.p4>

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

struct main_metadata_t {
    bit<32> port_in;
    bit<32> port_out;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    Counter<bit<10>, bit<12>>(1024, PNA_CounterType_t.PACKETS_AND_BYTES) counter0;
    Counter<bit<10>, bit<12>>(1024, PNA_CounterType_t.PACKETS) counter1;
    Counter<bit<10>, bit<12>>(1024, PNA_CounterType_t.BYTES) counter2;
    Register<bit<32>, bit<12>>(1024) reg;
    Meter<bit<12>>(1024, PNA_MeterType_t.BYTES) meter0;
    PNA_MeterColor_t color_out;
    PNA_MeterColor_t color_in = PNA_MeterColor_t.RED;
    action execute(bit<12> index) {
        color_out = meter0.execute(index, color_in, hdr.ipv4.totalLen);
        user_meta.port_out = (color_out == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
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

control MainDeparserImpl(packet_out pkt, in headers hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

