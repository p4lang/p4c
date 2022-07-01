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
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.color_out") PNA_MeterColor_t color_out_0;
    @name("MainControlImpl.color_in") PNA_MeterColor_t color_in_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.counter0") Counter<bit<10>, bit<12>>(32w1024, PNA_CounterType_t.PACKETS_AND_BYTES) counter0_0;
    @name("MainControlImpl.counter1") Counter<bit<10>, bit<12>>(32w1024, PNA_CounterType_t.PACKETS) counter1_0;
    @name("MainControlImpl.counter2") Counter<bit<10>, bit<12>>(32w1024, PNA_CounterType_t.BYTES) counter2_0;
    @name("MainControlImpl.reg") Register<bit<32>, bit<12>>(32w1024) reg_0;
    @name("MainControlImpl.meter0") Meter<bit<12>>(32w1024, PNA_MeterType_t.BYTES) meter0_0;
    @name("MainControlImpl.execute") action execute_1(@name("index") bit<12> index_1) {
        color_out_0 = meter0_0.execute(index_1, color_in_0, hdr.ipv4.totalLen);
        user_meta.port_out = (color_out_0 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0);
        reg_0.write(index_1, (color_out_0 == PNA_MeterColor_t.GREEN ? 32w1 : 32w0));
    }
    @name("MainControlImpl.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_1();
            execute_1();
        }
        default_action = NoAction_1();
    }
    @hidden action pnaexampledpdkexterns112() {
        counter0_0.count(12w1023, 32w20);
        counter1_0.count(12w512);
        counter2_0.count(12w1023, 32w64);
        user_meta.port_out = reg_0.read(12w1);
    }
    @hidden action pnaexampledpdkexterns94() {
        color_in_0 = PNA_MeterColor_t.RED;
    }
    @hidden table tbl_pnaexampledpdkexterns94 {
        actions = {
            pnaexampledpdkexterns94();
        }
        const default_action = pnaexampledpdkexterns94();
    }
    @hidden table tbl_pnaexampledpdkexterns112 {
        actions = {
            pnaexampledpdkexterns112();
        }
        const default_action = pnaexampledpdkexterns112();
    }
    apply {
        tbl_pnaexampledpdkexterns94.apply();
        if (user_meta.port_out == 32w1) {
            tbl_0.apply();
            tbl_pnaexampledpdkexterns112.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampledpdkexterns129() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnaexampledpdkexterns129 {
        actions = {
            pnaexampledpdkexterns129();
        }
        const default_action = pnaexampledpdkexterns129();
    }
    apply {
        tbl_pnaexampledpdkexterns129.apply();
    }
}

PNA_NIC<headers, main_metadata_t, headers, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

