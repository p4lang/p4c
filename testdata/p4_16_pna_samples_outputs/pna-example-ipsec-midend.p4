#include <core.p4>
#include <dpdk/pna.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> total_length;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragment_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> header_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header esp_t {
    bit<32> spi;
    bit<32> seq;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    esp_t      esp;
}

struct metadata_t {
    bit<32> next_hop_id;
    bool    bypass;
}

ipsec_accelerator() ipsec;
parser MainParserImpl(packet_in pkt, out headers_t hdrs, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("MainParserImpl.from_ipsec") bool from_ipsec_0;
    @name("MainParserImpl.status") ipsec_status status_0;
    state start {
        from_ipsec_0 = ipsec.from_ipsec(status_0);
        transition select((bit<1>)from_ipsec_0) {
            1w1: parse_ipv4;
            default: parse_ethernet;
        }
    }
    state parse_ethernet {
        pkt.extract<ethernet_t>(hdrs.ethernet);
        transition select(hdrs.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdrs.ipv4);
        transition select(hdrs.ipv4.protocol) {
            8w0x32: parse_esp;
            default: accept;
        }
    }
    state parse_esp {
        pkt.extract<esp_t>(hdrs.esp);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdrs, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.status") ipsec_status status_1;
    @name("MainControlImpl.status") ipsec_status status_2;
    @name("MainControlImpl.tmp") bool tmp;
    @name("MainControlImpl.tmp_0") bool tmp_0;
    @name("MainControlImpl.ipsec_enable") action ipsec_enable(@name("sa_index") bit<32> sa_index_1) {
        ipsec.enable();
        ipsec.set_sa_index<bit<32>>(sa_index_1);
        hdrs.ethernet.setInvalid();
    }
    @name("MainControlImpl.ipsec_enable") action ipsec_enable_1(@name("sa_index") bit<32> sa_index_2) {
        ipsec.enable();
        ipsec.set_sa_index<bit<32>>(sa_index_2);
        hdrs.ethernet.setInvalid();
    }
    @name("MainControlImpl.ipsec_bypass") action ipsec_bypass() {
        meta.bypass = true;
        ipsec.disable();
    }
    @name("MainControlImpl.ipsec_bypass") action ipsec_bypass_1() {
        meta.bypass = true;
        ipsec.disable();
    }
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_1() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_2() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_3() {
        drop_packet();
    }
    @name("MainControlImpl.inbound_table") table inbound_table_0 {
        key = {
            hdrs.ipv4.src_addr: exact @name("hdrs.ipv4.src_addr");
            hdrs.ipv4.dst_addr: exact @name("hdrs.ipv4.dst_addr");
            hdrs.esp.spi      : exact @name("hdrs.esp.spi");
        }
        actions = {
            ipsec_enable();
            ipsec_bypass();
            drop();
        }
        const default_action = drop();
    }
    @name("MainControlImpl.outbound_table") table outbound_table_0 {
        key = {
            hdrs.ipv4.src_addr: exact @name("hdrs.ipv4.src_addr");
            hdrs.ipv4.dst_addr: exact @name("hdrs.ipv4.dst_addr");
        }
        actions = {
            ipsec_enable_1();
            ipsec_bypass_1();
            drop_1();
        }
        default_action = ipsec_bypass_1();
    }
    @name("MainControlImpl.next_hop_id_set") action next_hop_id_set(@name("next_hop_id") bit<32> next_hop_id_1) {
        meta.next_hop_id = next_hop_id_1;
    }
    @name("MainControlImpl.routing_table") table routing_table_0 {
        key = {
            hdrs.ipv4.dst_addr: lpm @name("hdrs.ipv4.dst_addr");
        }
        actions = {
            next_hop_id_set();
            drop_2();
        }
        default_action = next_hop_id_set(32w0);
    }
    @name("MainControlImpl.next_hop_set") action next_hop_set(@name("dst_addr") bit<48> dst_addr_1, @name("src_addr") bit<48> src_addr_1, @name("ether_type") bit<16> ether_type_1, @name("port_id") bit<32> port_id) {
        hdrs.ethernet.setValid();
        hdrs.ethernet.dst_addr = dst_addr_1;
        hdrs.ethernet.src_addr = src_addr_1;
        hdrs.ethernet.ether_type = ether_type_1;
        send_to_port(port_id);
    }
    @name("MainControlImpl.next_hop_table") table next_hop_table_0 {
        key = {
            meta.next_hop_id: exact @name("meta.next_hop_id");
        }
        actions = {
            next_hop_set();
            drop_3();
        }
        default_action = drop_3();
    }
    @hidden action pnaexampleipsec248() {
        drop_packet();
    }
    @hidden action pnaexampleipsec241() {
        drop_packet();
    }
    @hidden action act() {
        tmp = ipsec.from_ipsec(status_1);
    }
    @hidden action pnaexampleipsec265() {
        drop_packet();
    }
    @hidden action pnaexampleipsec258() {
        drop_packet();
    }
    @hidden action act_0() {
        tmp_0 = ipsec.from_ipsec(status_2);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_pnaexampleipsec248 {
        actions = {
            pnaexampleipsec248();
        }
        const default_action = pnaexampleipsec248();
    }
    @hidden table tbl_pnaexampleipsec241 {
        actions = {
            pnaexampleipsec241();
        }
        const default_action = pnaexampleipsec241();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_pnaexampleipsec265 {
        actions = {
            pnaexampleipsec265();
        }
        const default_action = pnaexampleipsec265();
    }
    @hidden table tbl_pnaexampleipsec258 {
        actions = {
            pnaexampleipsec258();
        }
        const default_action = pnaexampleipsec258();
    }
    apply {
        if (istd.direction == PNA_Direction_t.NET_TO_HOST) {
            tbl_act.apply();
            if (tmp) {
                if (status_1 == ipsec_status.IPSEC_SUCCESS) {
                    routing_table_0.apply();
                    next_hop_table_0.apply();
                } else {
                    tbl_pnaexampleipsec248.apply();
                }
            } else if (hdrs.ipv4.isValid()) {
                if (hdrs.esp.isValid()) {
                    inbound_table_0.apply();
                    if (meta.bypass) {
                        routing_table_0.apply();
                        next_hop_table_0.apply();
                    }
                } else {
                    routing_table_0.apply();
                    next_hop_table_0.apply();
                }
            } else {
                tbl_pnaexampleipsec241.apply();
            }
        } else {
            tbl_act_0.apply();
            if (tmp_0) {
                if (status_2 == ipsec_status.IPSEC_SUCCESS) {
                    routing_table_0.apply();
                    next_hop_table_0.apply();
                } else {
                    tbl_pnaexampleipsec265.apply();
                }
            } else if (hdrs.ipv4.isValid()) {
                outbound_table_0.apply();
            } else {
                tbl_pnaexampleipsec258.apply();
            }
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdrs, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampleipsec281() {
        pkt.emit<ethernet_t>(hdrs.ethernet);
        pkt.emit<ipv4_t>(hdrs.ipv4);
        pkt.emit<esp_t>(hdrs.esp);
    }
    @hidden table tbl_pnaexampleipsec281 {
        actions = {
            pnaexampleipsec281();
        }
        const default_action = pnaexampleipsec281();
    }
    apply {
        tbl_pnaexampleipsec281.apply();
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
