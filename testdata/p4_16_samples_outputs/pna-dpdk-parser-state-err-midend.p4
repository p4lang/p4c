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

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct empty_metadata_t {
}

struct main_metadata_t {
    bit<1>                rng_result1;
    bit<16>               min1;
    bit<16>               max1;
    ExpireTimeProfileId_t timeout;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<tcp_t>(hdr.tcp);
        main_meta.rng_result1 = (bit<1>)(main_meta.min1 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= main_meta.max1);
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

struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    tcp_t hdr_3_tcp;
    @name(".do_range_checks_0") action do_range_checks_1(@name("min1") bit<16> min1_2, @name("max1") bit<16> max1_2) {
        hdr_3_tcp = hdr.tcp;
        user_meta.rng_result1 = (bit<1>)(min1_2 <= hdr_3_tcp.srcPort && hdr_3_tcp.srcPort <= max1_2);
    }
    @name("MainControlImpl.next_hop") action next_hop(@name("vport") PortId_t vport) {
        send_to_port(vport);
    }
    @name("MainControlImpl.add_on_miss_action") action add_on_miss_action() {
        add_entry<bit<32>>(action_name = "next_hop", action_params = 32w0, expire_time_profile_id = user_meta.timeout);
    }
    @name("MainControlImpl.do_range_checks_1") action do_range_checks_2(@name("min1") bit<16> min1_3, @name("max1") bit<16> max1_3) {
        user_meta.rng_result1 = (bit<1>)(min1_3 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= max1_3);
    }
    @name("MainControlImpl.ipv4_da") table ipv4_da_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            @tableonly next_hop();
            @defaultonly add_on_miss_action();
        }
        add_on_miss = true;
        const default_action = add_on_miss_action();
    }
    @name("MainControlImpl.next_hop2") action next_hop2(@name("vport") PortId_t vport_2, @name("newAddr") bit<32> newAddr) {
        send_to_port(vport_2);
        hdr.ipv4.srcAddr = newAddr;
    }
    @name("MainControlImpl.add_on_miss_action2") action add_on_miss_action2() {
        add_entry<tuple_0>(action_name = "next_hop", action_params = (tuple_0){f0 = 32w0,f1 = 32w1234}, expire_time_profile_id = user_meta.timeout);
    }
    @name("MainControlImpl.ipv4_da2") table ipv4_da2_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            @tableonly next_hop2();
            @defaultonly add_on_miss_action2();
            do_range_checks_2();
            do_range_checks_1();
        }
        add_on_miss = true;
        const default_action = add_on_miss_action2();
    }
    @hidden action pnadpdkparserstateerr191() {
        user_meta.rng_result1 = (bit<1>)(16w100 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= 16w200);
    }
    @hidden table tbl_pnadpdkparserstateerr191 {
        actions = {
            pnadpdkparserstateerr191();
        }
        const default_action = pnadpdkparserstateerr191();
    }
    apply {
        tbl_pnadpdkparserstateerr191.apply();
        if (hdr.ipv4.isValid()) {
            ipv4_da_0.apply();
            ipv4_da2_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkparserstateerr206() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadpdkparserstateerr206 {
        actions = {
            pnadpdkparserstateerr206();
        }
        const default_action = pnadpdkparserstateerr206();
    }
    apply {
        tbl_pnadpdkparserstateerr206.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

