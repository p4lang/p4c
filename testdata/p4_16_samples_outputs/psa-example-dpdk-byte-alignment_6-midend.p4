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

struct s1_t {
    bit<4> f1;
    bit<4> f2;
}

header ipv4_t {
    bit<4>  _version0;
    bit<4>  _ihl1;
    bit<8>  _diffserv2;
    bit<32> _totalLen3;
    bit<16> _identification4;
    bit<3>  _flags5;
    bit<13> _fragOffset6;
    bit<8>  _ttl7;
    bit<8>  _protocol8;
    bit<16> _hdrChecksum9;
    bit<32> _srcAddr10;
    bit<32> _dstAddr11;
    bit<4>  _s1_f112;
    bit<4>  _s1_f213;
}

struct empty_metadata_t {
}

struct metadata_t {
    bit<32> port_in;
    bit<32> port_out;
    bit<4>  temp;
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
        verify(hdr.ipv4._ihl1 == 4w5, error.UnhandledIPv4Options);
        transition select(hdr.ipv4._version0) {
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.color_out") PSA_MeterColor_t color_out_0;
    @name("ingress.color_in") PSA_MeterColor_t color_in_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.counter0") Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.PACKETS_AND_BYTES) counter0_0;
    @name("ingress.counter1") Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.PACKETS) counter1_0;
    @name("ingress.counter2") Counter<bit<10>, bit<12>>(32w1024, PSA_CounterType_t.BYTES) counter2_0;
    @name("ingress.reg") Register<bit<32>, bit<12>>(32w1024) reg_0;
    @name("ingress.meter0") Meter<bit<12>>(32w1024, PSA_MeterType_t.BYTES) meter0_0;
    @name("ingress.execute") action execute_1(@name("index") bit<12> index_1) {
        hdr.ipv4._ihl1 = 4w5;
        color_out_0 = meter0_0.execute(index_1, color_in_0, hdr.ipv4._totalLen3);
        user_meta.port_out = (color_out_0 == PSA_MeterColor_t.GREEN ? 32w1 : 32w0);
        reg_0.write(index_1, (color_out_0 == PSA_MeterColor_t.GREEN ? 32w1 : 32w0));
        hdr.ipv4._ihl1 = (hdr.ipv4._version0 == 4w6 ? 4w6 : 4w5);
        hdr.ipv4._s1_f112 = (user_meta.temp == 4w6 ? 4w7 : hdr.ipv4._s1_f112);
        hdr.ipv4._s1_f213 = (user_meta.temp == 4w6 ? 4w8 : hdr.ipv4._s1_f213);
        hdr.ipv4._s1_f112 = 4w7;
        hdr.ipv4._s1_f213 = (user_meta.temp == 4w6 ? 4w8 : 4w9);
    }
    @name("ingress.test") action test() {
        hdr.ipv4._hdrChecksum9[3:0] = (hdr.ipv4._version0 == 4w4 ? hdr.ipv4._version0 + 4w5 : hdr.ipv4._hdrChecksum9[3:0]);
    }
    @name("ingress.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        actions = {
            NoAction_1();
            execute_1();
        }
        default_action = NoAction_1();
    }
    @hidden action psaexampledpdkbytealignment_6l109() {
        counter0_0.count(12w1023, 32w20);
        counter1_0.count(12w512);
        counter2_0.count(12w1023, 32w64);
        user_meta.port_out = reg_0.read(12w1);
    }
    @hidden action psaexampledpdkbytealignment_6l77() {
        color_in_0 = PSA_MeterColor_t.RED;
    }
    @hidden table tbl_psaexampledpdkbytealignment_6l77 {
        actions = {
            psaexampledpdkbytealignment_6l77();
        }
        const default_action = psaexampledpdkbytealignment_6l77();
    }
    @hidden table tbl_psaexampledpdkbytealignment_6l109 {
        actions = {
            psaexampledpdkbytealignment_6l109();
        }
        const default_action = psaexampledpdkbytealignment_6l109();
    }
    @hidden table tbl_test {
        actions = {
            test();
        }
        const default_action = test();
    }
    apply {
        tbl_psaexampledpdkbytealignment_6l77.apply();
        if (user_meta.port_out == 32w1) {
            tbl_0.apply();
            tbl_psaexampledpdkbytealignment_6l109.apply();
            tbl_test.apply();
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
    @hidden action psaexampledpdkbytealignment_6l133() {
        hdr.ipv4._hdrChecksum9 = 16w4;
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_psaexampledpdkbytealignment_6l133 {
        actions = {
            psaexampledpdkbytealignment_6l133();
        }
        const default_action = psaexampledpdkbytealignment_6l133();
    }
    apply {
        tbl_psaexampledpdkbytealignment_6l133.apply();
    }
}

control EgressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata_t, headers, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

