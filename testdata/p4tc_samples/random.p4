#include <core.p4>
#include <tc/pna.p4>

struct my_ingress_metadata_t {
}

typedef bit<48> EthernetAddress;

header ethernet_t {
    @tc_type("macaddr") EthernetAddress dstAddr;
    @tc_type("macaddr") EthernetAddress srcAddr;
    bit<16>         etherType;
}

header random_t {
    bit<16> f1;
    bit<32> f2;
    bit<16> f3;
}

// User-defined struct containing all of those headers parsed in the
// main parser.
struct headers_t {
    ethernet_t eth;
    random_t   rand;
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t hdr,
    inout my_ingress_metadata_t meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.eth);
        pkt.extract(hdr.rand);
        transition accept;
    }
}

control Main(inout headers_t hdr,                 // from main parser
             inout my_ingress_metadata_t meta,               // from main parser, to "next block"
             in    pna_main_input_metadata_t istd,
             inout pna_main_output_metadata_t ostd) {

    Random(16w0, 16w127) r1;
    Random<bit<32>>(0x80_00_00_01, 0x80_00_00_05) r2;
    Random(16w256, 16w259) r3;

    action send_nh(@tc_type("dev") PortId_t port_id, @tc_type("macaddr") bit<48> dmac, @tc_type("macaddr") bit<48> smac) {
        hdr.eth.srcAddr = smac;
        hdr.eth.dstAddr = dmac;
        hdr.rand.f3 = r3.read();
        send_to_port(port_id);
    }
    action drop() {
        drop_packet();
    }

    table nh_table {
        key = {
            istd.input_port : exact;
        }
        actions = {
            send_nh;
            drop;
        }
        size = 100;
        const default_action = drop;
    }

    apply {
        hdr.rand.f1 = r1.read();
        hdr.rand.f2 = r2.read();
        nh_table.apply();
    }
}

control MainDeparserImpl(
    packet_out pkt,
    inout headers_t hdr,
    in my_ingress_metadata_t meta,
    in pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.eth);
        pkt.emit(hdr.rand);
    }
}

// BEGIN:Package_Instantiation_Example
PNA_NIC(
    MainParserImpl(),
    Main(),
    MainDeparserImpl()
    ) main;
// END:Package_Instantiation_Example
