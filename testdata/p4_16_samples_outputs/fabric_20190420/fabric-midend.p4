#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct int_metadata_t {
    bool    source;
    bool    transit;
    bool    sink;
    bit<32> switch_id;
    bit<8>  new_words;
    bit<16> new_bytes;
    bit<32> ig_tstamp;
    bit<32> eg_tstamp;
}

header int_header_t {
    bit<2>  ver;
    bit<2>  rep;
    bit<1>  c;
    bit<1>  e;
    bit<5>  rsvd1;
    bit<5>  ins_cnt;
    bit<8>  max_hop_cnt;
    bit<8>  total_hop_cnt;
    bit<4>  instruction_mask_0003;
    bit<4>  instruction_mask_0407;
    bit<4>  instruction_mask_0811;
    bit<4>  instruction_mask_1215;
    bit<16> rsvd2;
}

header intl4_shim_t {
    bit<8> int_type;
    bit<8> rsvd1;
    bit<8> len_words;
    bit<8> rsvd2;
}

header intl4_tail_t {
    bit<8>  next_proto;
    bit<16> dest_port;
    bit<2>  padding;
    bit<6>  dscp;
}

@controller_header("packet_in") header packet_in_header_t {
    bit<9> ingress_port;
    bit<7> _pad;
}

@controller_header("packet_out") header packet_out_header_t {
    bit<9> egress_port;
    bit<7> _pad;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header vlan_tag_t {
    bit<3>  pri;
    bit<1>  cfi;
    bit<12> vlan_id;
    bit<16> eth_type;
}

header mpls_t {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header ipv6_t {
    bit<4>   version;
    bit<8>   traffic_class;
    bit<20>  flow_label;
    bit<16>  payload_len;
    bit<8>   next_hdr;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
}

header tcp_t {
    bit<16> sport;
    bit<16> dport;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_t {
    bit<16> sport;
    bit<16> dport;
    bit<16> len;
    bit<16> checksum;
}

header icmp_t {
    bit<8>  icmp_type;
    bit<8>  icmp_code;
    bit<16> checksum;
    bit<16> identifier;
    bit<16> sequence_number;
    bit<64> timestamp;
}

header gtpu_t {
    bit<3>  version;
    bit<1>  pt;
    bit<1>  spare;
    bit<1>  ex_flag;
    bit<1>  seq_flag;
    bit<1>  npdu_flag;
    bit<8>  msgtype;
    bit<16> msglen;
    bit<32> teid;
}

struct spgw_meta_t {
    bit<2>  direction;
    bit<16> ipv4_len;
    bit<32> teid;
    bit<32> s1u_enb_addr;
    bit<32> s1u_sgw_addr;
}

struct fabric_metadata_t {
    bit<16> _eth_type0;
    bit<16> _ip_eth_type1;
    bit<12> _vlan_id2;
    bit<3>  _vlan_pri3;
    bit<1>  _vlan_cfi4;
    bit<20> _mpls_label5;
    bit<8>  _mpls_ttl6;
    bool    _skip_forwarding7;
    bool    _skip_next8;
    bit<3>  _fwd_type9;
    bit<32> _next_id10;
    bool    _is_multicast11;
    bool    _is_controller_packet_out12;
    bool    _clone_to_cpu13;
    bit<8>  _ip_proto14;
    bit<16> _l4_sport15;
    bit<16> _l4_dport16;
    bit<2>  _spgw_direction17;
    bit<16> _spgw_ipv4_len18;
    bit<32> _spgw_teid19;
    bit<32> _spgw_s1u_enb_addr20;
    bit<32> _spgw_s1u_sgw_addr21;
}

struct parsed_headers_t {
    ethernet_t          ethernet;
    vlan_tag_t          vlan_tag;
    vlan_tag_t          inner_vlan_tag;
    mpls_t              mpls;
    ipv4_t              gtpu_ipv4;
    udp_t               gtpu_udp;
    gtpu_t              gtpu;
    ipv4_t              inner_ipv4;
    udp_t               inner_udp;
    ipv4_t              ipv4;
    tcp_t               tcp;
    udp_t               udp;
    icmp_t              icmp;
    packet_out_header_t packet_out;
    packet_in_header_t  packet_in;
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<6>  f2;
    bit<2>  f3;
    bit<16> f4;
    bit<16> f5;
    bit<3>  f6;
    bit<13> f7;
    bit<8>  f8;
    bit<8>  f9;
    bit<32> f10;
    bit<32> f11;
}

control FabricComputeChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.dscp,f3 = hdr.ipv4.ecn,f4 = hdr.ipv4.total_len,f5 = hdr.ipv4.identification,f6 = hdr.ipv4.flags,f7 = hdr.ipv4.frag_offset,f8 = hdr.ipv4.ttl,f9 = hdr.ipv4.protocol,f10 = hdr.ipv4.src_addr,f11 = hdr.ipv4.dst_addr}, hdr.ipv4.hdr_checksum, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(hdr.gtpu_ipv4.isValid(), (tuple_0){f0 = hdr.gtpu_ipv4.version,f1 = hdr.gtpu_ipv4.ihl,f2 = hdr.gtpu_ipv4.dscp,f3 = hdr.gtpu_ipv4.ecn,f4 = hdr.gtpu_ipv4.total_len,f5 = hdr.gtpu_ipv4.identification,f6 = hdr.gtpu_ipv4.flags,f7 = hdr.gtpu_ipv4.frag_offset,f8 = hdr.gtpu_ipv4.ttl,f9 = hdr.gtpu_ipv4.protocol,f10 = hdr.gtpu_ipv4.src_addr,f11 = hdr.gtpu_ipv4.dst_addr}, hdr.gtpu_ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

control FabricVerifyChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.dscp,f3 = hdr.ipv4.ecn,f4 = hdr.ipv4.total_len,f5 = hdr.ipv4.identification,f6 = hdr.ipv4.flags,f7 = hdr.ipv4.frag_offset,f8 = hdr.ipv4.ttl,f9 = hdr.ipv4.protocol,f10 = hdr.ipv4.src_addr,f11 = hdr.ipv4.dst_addr}, hdr.ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

parser FabricParser(packet_in packet, out parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    @name("FabricParser.tmp_0") bit<4> tmp_0;
    state start {
        transition select(standard_metadata.ingress_port) {
            9w255: parse_packet_out;
            default: parse_ethernet;
        }
    }
    state parse_packet_out {
        packet.extract<packet_out_header_t>(hdr.packet_out);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        fabric_metadata._eth_type0 = hdr.ethernet.eth_type;
        fabric_metadata._vlan_id2 = 12w4094;
        transition select(hdr.ethernet.eth_type) {
            16w0x8100: parse_vlan_tag;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_vlan_tag {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition select(hdr.vlan_tag.eth_type) {
            16w0x800: parse_ipv4;
            16w0x8847: parse_mpls;
            16w0x8100: parse_inner_vlan_tag;
            default: accept;
        }
    }
    state parse_inner_vlan_tag {
        packet.extract<vlan_tag_t>(hdr.inner_vlan_tag);
        transition select(hdr.inner_vlan_tag.eth_type) {
            16w0x800: parse_ipv4;
            16w0x8847: parse_mpls;
            default: accept;
        }
    }
    state parse_mpls {
        packet.extract<mpls_t>(hdr.mpls);
        fabric_metadata._mpls_label5 = hdr.mpls.label;
        fabric_metadata._mpls_ttl6 = hdr.mpls.ttl;
        tmp_0 = packet.lookahead<bit<4>>();
        transition select(tmp_0) {
            4w4: parse_ipv4;
            default: parse_ethernet;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        fabric_metadata._ip_proto14 = hdr.ipv4.protocol;
        fabric_metadata._ip_eth_type1 = 16w0x800;
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            8w1: parse_icmp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        fabric_metadata._l4_sport15 = hdr.tcp.sport;
        fabric_metadata._l4_dport16 = hdr.tcp.dport;
        transition accept;
    }
    state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        fabric_metadata._l4_sport15 = hdr.udp.sport;
        fabric_metadata._l4_dport16 = hdr.udp.dport;
        transition select(hdr.udp.dport) {
            16w2152: parse_gtpu;
            default: accept;
        }
    }
    state parse_icmp {
        packet.extract<icmp_t>(hdr.icmp);
        transition accept;
    }
    state parse_gtpu {
        transition select(hdr.ipv4.dst_addr[31:24]) {
            8w140: do_parse_gtpu;
            default: accept;
        }
    }
    state do_parse_gtpu {
        packet.extract<gtpu_t>(hdr.gtpu);
        packet.extract<ipv4_t>(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_inner_udp;
            8w1: parse_icmp;
            default: accept;
        }
    }
    state parse_inner_udp {
        packet.extract<udp_t>(hdr.inner_udp);
        fabric_metadata._l4_sport15 = hdr.inner_udp.sport;
        fabric_metadata._l4_dport16 = hdr.inner_udp.dport;
        transition accept;
    }
}

control FabricDeparser(packet_out packet, in parsed_headers_t hdr) {
    apply {
        packet.emit<packet_in_header_t>(hdr.packet_in);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
        packet.emit<vlan_tag_t>(hdr.inner_vlan_tag);
        packet.emit<mpls_t>(hdr.mpls);
        packet.emit<ipv4_t>(hdr.gtpu_ipv4);
        packet.emit<udp_t>(hdr.gtpu_udp);
        packet.emit<gtpu_t>(hdr.gtpu);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<icmp_t>(hdr.icmp);
    }
}

control FabricIngress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    bool hasExited;
    @name("FabricIngress.spgw_normalizer.hasReturned") bool spgw_normalizer_hasReturned;
    @name("FabricIngress.spgw_ingress.hasReturned_0") bool spgw_ingress_hasReturned;
    @name(".nop") action nop_2() {
    }
    @name(".nop") action nop_3() {
    }
    @name(".nop") action nop_4() {
    }
    @name(".nop") action nop_5() {
    }
    @name(".nop") action nop_6() {
    }
    @name(".nop") action nop_7() {
    }
    @name(".nop") action nop_8() {
    }
    @name(".nop") action nop_9() {
    }
    @name(".nop") action nop_10() {
    }
    @name("FabricIngress.filtering.ingress_port_vlan_counter") direct_counter(CounterType.packets_and_bytes) filtering_ingress_port_vlan_counter;
    @name("FabricIngress.filtering.deny") action filtering_deny_0() {
        fabric_metadata._skip_forwarding7 = true;
        fabric_metadata._skip_next8 = true;
        filtering_ingress_port_vlan_counter.count();
    }
    @name("FabricIngress.filtering.permit") action filtering_permit_0() {
        filtering_ingress_port_vlan_counter.count();
    }
    @name("FabricIngress.filtering.permit_with_internal_vlan") action filtering_permit_with_internal_vlan_0(@name("vlan_id") bit<12> vlan_id_2) {
        fabric_metadata._vlan_id2 = vlan_id_2;
        filtering_ingress_port_vlan_counter.count();
    }
    @name("FabricIngress.filtering.ingress_port_vlan") table filtering_ingress_port_vlan {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port");
            hdr.vlan_tag.isValid()        : exact @name("vlan_is_valid");
            hdr.vlan_tag.vlan_id          : ternary @name("vlan_id");
        }
        actions = {
            filtering_deny_0();
            filtering_permit_0();
            filtering_permit_with_internal_vlan_0();
        }
        const default_action = filtering_deny_0();
        counters = filtering_ingress_port_vlan_counter;
        size = 1024;
    }
    @name("FabricIngress.filtering.fwd_classifier_counter") direct_counter(CounterType.packets_and_bytes) filtering_fwd_classifier_counter;
    @name("FabricIngress.filtering.set_forwarding_type") action filtering_set_forwarding_type_0(@name("fwd_type") bit<3> fwd_type_1) {
        fabric_metadata._fwd_type9 = fwd_type_1;
        filtering_fwd_classifier_counter.count();
    }
    @name("FabricIngress.filtering.fwd_classifier") table filtering_fwd_classifier {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port");
            hdr.ethernet.dst_addr         : ternary @name("eth_dst");
            fabric_metadata._eth_type0    : exact @name("eth_type");
        }
        actions = {
            filtering_set_forwarding_type_0();
        }
        const default_action = filtering_set_forwarding_type_0(3w0);
        counters = filtering_fwd_classifier_counter;
        size = 1024;
    }
    @name("FabricIngress.forwarding.bridging_counter") direct_counter(CounterType.packets_and_bytes) forwarding_bridging_counter;
    @name("FabricIngress.forwarding.set_next_id_bridging") action forwarding_set_next_id_bridging_0(@name("next_id") bit<32> next_id_0) {
        fabric_metadata._next_id10 = next_id_0;
        forwarding_bridging_counter.count();
    }
    @name("FabricIngress.forwarding.bridging") table forwarding_bridging {
        key = {
            fabric_metadata._vlan_id2: exact @name("vlan_id");
            hdr.ethernet.dst_addr    : ternary @name("eth_dst");
        }
        actions = {
            forwarding_set_next_id_bridging_0();
            @defaultonly nop_2();
        }
        const default_action = nop_2();
        counters = forwarding_bridging_counter;
        size = 1024;
    }
    @name("FabricIngress.forwarding.mpls_counter") direct_counter(CounterType.packets_and_bytes) forwarding_mpls_counter;
    @name("FabricIngress.forwarding.pop_mpls_and_next") action forwarding_pop_mpls_and_next_0(@name("next_id") bit<32> next_id_6) {
        fabric_metadata._mpls_label5 = 20w0;
        fabric_metadata._next_id10 = next_id_6;
        forwarding_mpls_counter.count();
    }
    @name("FabricIngress.forwarding.mpls") table forwarding_mpls {
        key = {
            fabric_metadata._mpls_label5: exact @name("mpls_label");
        }
        actions = {
            forwarding_pop_mpls_and_next_0();
            @defaultonly nop_3();
        }
        const default_action = nop_3();
        counters = forwarding_mpls_counter;
        size = 1024;
    }
    @name("FabricIngress.forwarding.routing_v4_counter") direct_counter(CounterType.packets_and_bytes) forwarding_routing_v4_counter;
    @name("FabricIngress.forwarding.set_next_id_routing_v4") action forwarding_set_next_id_routing_v4_0(@name("next_id") bit<32> next_id_7) {
        fabric_metadata._next_id10 = next_id_7;
        forwarding_routing_v4_counter.count();
    }
    @name("FabricIngress.forwarding.nop_routing_v4") action forwarding_nop_routing_v4_0() {
        forwarding_routing_v4_counter.count();
    }
    @name("FabricIngress.forwarding.routing_v4") table forwarding_routing_v4 {
        key = {
            hdr.ipv4.dst_addr: lpm @name("ipv4_dst");
        }
        actions = {
            forwarding_set_next_id_routing_v4_0();
            forwarding_nop_routing_v4_0();
            @defaultonly nop_4();
        }
        const default_action = nop_4();
        counters = forwarding_routing_v4_counter;
        size = 1024;
    }
    @name("FabricIngress.acl.acl_counter") direct_counter(CounterType.packets_and_bytes) acl_acl_counter;
    @name("FabricIngress.acl.set_next_id_acl") action acl_set_next_id_acl_0(@name("next_id") bit<32> next_id_8) {
        fabric_metadata._next_id10 = next_id_8;
        acl_acl_counter.count();
    }
    @name("FabricIngress.acl.punt_to_cpu") action acl_punt_to_cpu_0() {
        standard_metadata.egress_spec = 9w255;
        fabric_metadata._skip_next8 = true;
        acl_acl_counter.count();
    }
    @name("FabricIngress.acl.clone_to_cpu") action acl_clone_to_cpu_0() {
        fabric_metadata._clone_to_cpu13 = true;
        acl_acl_counter.count();
    }
    @name("FabricIngress.acl.drop") action acl_drop_0() {
        mark_to_drop(standard_metadata);
        fabric_metadata._skip_next8 = true;
        acl_acl_counter.count();
    }
    @name("FabricIngress.acl.nop_acl") action acl_nop_acl_0() {
        acl_acl_counter.count();
    }
    @name("FabricIngress.acl.acl") table acl_acl {
        key = {
            standard_metadata.ingress_port: ternary @name("ig_port");
            fabric_metadata._ip_proto14   : ternary @name("ip_proto");
            fabric_metadata._l4_sport15   : ternary @name("l4_sport");
            fabric_metadata._l4_dport16   : ternary @name("l4_dport");
            hdr.ethernet.dst_addr         : ternary @name("eth_src");
            hdr.ethernet.src_addr         : ternary @name("eth_dst");
            hdr.vlan_tag.vlan_id          : ternary @name("vlan_id");
            fabric_metadata._eth_type0    : ternary @name("eth_type");
            hdr.ipv4.src_addr             : ternary @name("ipv4_src");
            hdr.ipv4.dst_addr             : ternary @name("ipv4_dst");
            hdr.icmp.icmp_type            : ternary @name("icmp_type");
            hdr.icmp.icmp_code            : ternary @name("icmp_code");
        }
        actions = {
            acl_set_next_id_acl_0();
            acl_punt_to_cpu_0();
            acl_clone_to_cpu_0();
            acl_drop_0();
            acl_nop_acl_0();
        }
        const default_action = acl_nop_acl_0();
        size = 1024;
        counters = acl_acl_counter;
    }
    @name("FabricIngress.next.next_vlan_counter") direct_counter(CounterType.packets_and_bytes) next_next_vlan_counter;
    @name("FabricIngress.next.set_vlan") action next_set_vlan_0(@name("vlan_id") bit<12> vlan_id_3) {
        fabric_metadata._vlan_id2 = vlan_id_3;
        next_next_vlan_counter.count();
    }
    @name("FabricIngress.next.next_vlan") table next_next_vlan {
        key = {
            fabric_metadata._next_id10: exact @name("next_id");
        }
        actions = {
            next_set_vlan_0();
            @defaultonly nop_5();
        }
        const default_action = nop_5();
        counters = next_next_vlan_counter;
        size = 1024;
    }
    @name("FabricIngress.next.xconnect_counter") direct_counter(CounterType.packets_and_bytes) next_xconnect_counter;
    @name("FabricIngress.next.output_xconnect") action next_output_xconnect_0(@name("port_num") bit<9> port_num) {
        standard_metadata.egress_spec = port_num;
        next_xconnect_counter.count();
    }
    @name("FabricIngress.next.set_next_id_xconnect") action next_set_next_id_xconnect_0(@name("next_id") bit<32> next_id_9) {
        fabric_metadata._next_id10 = next_id_9;
        next_xconnect_counter.count();
    }
    @name("FabricIngress.next.xconnect") table next_xconnect {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port");
            fabric_metadata._next_id10    : exact @name("next_id");
        }
        actions = {
            next_output_xconnect_0();
            next_set_next_id_xconnect_0();
            @defaultonly nop_6();
        }
        counters = next_xconnect_counter;
        const default_action = nop_6();
        size = 1024;
    }
    @max_group_size(16) @name("FabricIngress.next.hashed_selector") action_selector(HashAlgorithm.crc16, 32w1024, 32w16) next_hashed_selector;
    @name("FabricIngress.next.hashed_counter") direct_counter(CounterType.packets_and_bytes) next_hashed_counter;
    @name("FabricIngress.next.output_hashed") action next_output_hashed_0(@name("port_num") bit<9> port_num_0) {
        standard_metadata.egress_spec = port_num_0;
        next_hashed_counter.count();
    }
    @name("FabricIngress.next.routing_hashed") action next_routing_hashed_0(@name("port_num") bit<9> port_num_1, @name("smac") bit<48> smac, @name("dmac") bit<48> dmac) {
        hdr.ethernet.src_addr = smac;
        hdr.ethernet.dst_addr = dmac;
        standard_metadata.egress_spec = port_num_1;
        next_hashed_counter.count();
    }
    @name("FabricIngress.next.mpls_routing_hashed") action next_mpls_routing_hashed_0(@name("port_num") bit<9> port_num_2, @name("smac") bit<48> smac_0, @name("dmac") bit<48> dmac_0, @name("label") bit<20> label_0) {
        fabric_metadata._mpls_label5 = label_0;
        hdr.ethernet.src_addr = smac_0;
        hdr.ethernet.dst_addr = dmac_0;
        standard_metadata.egress_spec = port_num_2;
        next_hashed_counter.count();
    }
    @name("FabricIngress.next.hashed") table next_hashed {
        key = {
            fabric_metadata._next_id10 : exact @name("next_id");
            hdr.ipv4.dst_addr          : selector @name("hdr.ipv4.dst_addr");
            hdr.ipv4.src_addr          : selector @name("hdr.ipv4.src_addr");
            fabric_metadata._ip_proto14: selector @name("fabric_metadata.ip_proto");
            fabric_metadata._l4_sport15: selector @name("fabric_metadata.l4_sport");
            fabric_metadata._l4_dport16: selector @name("fabric_metadata.l4_dport");
        }
        actions = {
            next_output_hashed_0();
            next_routing_hashed_0();
            next_mpls_routing_hashed_0();
            @defaultonly nop_7();
        }
        implementation = next_hashed_selector;
        counters = next_hashed_counter;
        const default_action = nop_7();
        size = 1024;
    }
    @name("FabricIngress.next.multicast_counter") direct_counter(CounterType.packets_and_bytes) next_multicast_counter;
    @name("FabricIngress.next.set_mcast_group_id") action next_set_mcast_group_id_0(@name("group_id") bit<16> group_id) {
        standard_metadata.mcast_grp = group_id;
        fabric_metadata._is_multicast11 = true;
        next_multicast_counter.count();
    }
    @name("FabricIngress.next.multicast") table next_multicast {
        key = {
            fabric_metadata._next_id10: exact @name("next_id");
        }
        actions = {
            next_set_mcast_group_id_0();
            @defaultonly nop_8();
        }
        counters = next_multicast_counter;
        const default_action = nop_8();
        size = 1024;
    }
    @name("FabricIngress.port_counters_control.egress_port_counter") counter(32w511, CounterType.packets_and_bytes) port_counters_control_egress_port_counter;
    @name("FabricIngress.port_counters_control.ingress_port_counter") counter(32w511, CounterType.packets_and_bytes) port_counters_control_ingress_port_counter;
    @name("FabricIngress.spgw_ingress.ue_counter") direct_counter(CounterType.packets_and_bytes) spgw_ingress_ue_counter;
    @hidden @name("FabricIngress.spgw_ingress.gtpu_decap") action spgw_ingress_gtpu_decap_0() {
        hdr.gtpu_ipv4.setInvalid();
        hdr.gtpu_udp.setInvalid();
        hdr.gtpu.setInvalid();
    }
    @name("FabricIngress.spgw_ingress.set_dl_sess_info") action spgw_ingress_set_dl_sess_info_0(@name("teid") bit<32> teid_1, @name("s1u_enb_addr") bit<32> s1u_enb_addr_1, @name("s1u_sgw_addr") bit<32> s1u_sgw_addr_1) {
        fabric_metadata._spgw_teid19 = teid_1;
        fabric_metadata._spgw_s1u_enb_addr20 = s1u_enb_addr_1;
        fabric_metadata._spgw_s1u_sgw_addr21 = s1u_sgw_addr_1;
        spgw_ingress_ue_counter.count();
    }
    @name("FabricIngress.spgw_ingress.dl_sess_lookup") table spgw_ingress_dl_sess_lookup {
        key = {
            hdr.ipv4.dst_addr: exact @name("ipv4_dst");
        }
        actions = {
            spgw_ingress_set_dl_sess_info_0();
            @defaultonly nop_9();
        }
        const default_action = nop_9();
        counters = spgw_ingress_ue_counter;
    }
    @name("FabricIngress.spgw_ingress.s1u_filter_table") table spgw_ingress_s1u_filter_table {
        key = {
            hdr.gtpu_ipv4.dst_addr: exact @name("gtp_ipv4_dst");
        }
        actions = {
            nop_10();
        }
        const default_action = nop_10();
    }
    @hidden action spgw30() {
        spgw_normalizer_hasReturned = true;
    }
    @hidden action fabric78() {
        hasExited = false;
        hdr.gtpu_ipv4.setInvalid();
        hdr.gtpu_udp.setInvalid();
        spgw_normalizer_hasReturned = false;
    }
    @hidden action spgw35() {
        hdr.udp = hdr.inner_udp;
    }
    @hidden action spgw37() {
        hdr.udp.setInvalid();
    }
    @hidden action spgw31() {
        hdr.gtpu_ipv4 = hdr.ipv4;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.gtpu_udp = hdr.udp;
    }
    @hidden action packetio25() {
        standard_metadata.egress_spec = hdr.packet_out.egress_port;
        hdr.packet_out.setInvalid();
        fabric_metadata._is_controller_packet_out12 = true;
        hasExited = true;
    }
    @hidden action filtering105() {
        fabric_metadata._eth_type0 = hdr.vlan_tag.eth_type;
        fabric_metadata._vlan_id2 = hdr.vlan_tag.vlan_id;
        fabric_metadata._vlan_pri3 = hdr.vlan_tag.pri;
        fabric_metadata._vlan_cfi4 = hdr.vlan_tag.cfi;
    }
    @hidden action filtering115() {
        fabric_metadata._mpls_ttl6 = 8w65;
    }
    @hidden action spgw149() {
        mark_to_drop(standard_metadata);
    }
    @hidden action spgw151() {
        fabric_metadata._spgw_direction17 = 2w1;
    }
    @hidden action spgw154() {
        fabric_metadata._spgw_direction17 = 2w2;
    }
    @hidden action spgw156() {
        fabric_metadata._spgw_direction17 = 2w0;
        spgw_ingress_hasReturned = true;
    }
    @hidden action act() {
        spgw_ingress_hasReturned = false;
    }
    @hidden action spgw175() {
        fabric_metadata._spgw_ipv4_len18 = hdr.ipv4.total_len;
    }
    @hidden action port_counter31() {
        port_counters_control_egress_port_counter.count((bit<32>)standard_metadata.egress_spec);
    }
    @hidden action port_counter34() {
        port_counters_control_ingress_port_counter.count((bit<32>)standard_metadata.ingress_port);
    }
    @hidden table tbl_fabric78 {
        actions = {
            fabric78();
        }
        const default_action = fabric78();
    }
    @hidden table tbl_spgw30 {
        actions = {
            spgw30();
        }
        const default_action = spgw30();
    }
    @hidden table tbl_spgw31 {
        actions = {
            spgw31();
        }
        const default_action = spgw31();
    }
    @hidden table tbl_spgw35 {
        actions = {
            spgw35();
        }
        const default_action = spgw35();
    }
    @hidden table tbl_spgw37 {
        actions = {
            spgw37();
        }
        const default_action = spgw37();
    }
    @hidden table tbl_packetio25 {
        actions = {
            packetio25();
        }
        const default_action = packetio25();
    }
    @hidden table tbl_filtering105 {
        actions = {
            filtering105();
        }
        const default_action = filtering105();
    }
    @hidden table tbl_filtering115 {
        actions = {
            filtering115();
        }
        const default_action = filtering115();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_spgw149 {
        actions = {
            spgw149();
        }
        const default_action = spgw149();
    }
    @hidden table tbl_spgw151 {
        actions = {
            spgw151();
        }
        const default_action = spgw151();
    }
    @hidden table tbl_spgw_ingress_gtpu_decap {
        actions = {
            spgw_ingress_gtpu_decap_0();
        }
        const default_action = spgw_ingress_gtpu_decap_0();
    }
    @hidden table tbl_spgw154 {
        actions = {
            spgw154();
        }
        const default_action = spgw154();
    }
    @hidden table tbl_spgw156 {
        actions = {
            spgw156();
        }
        const default_action = spgw156();
    }
    @hidden table tbl_spgw175 {
        actions = {
            spgw175();
        }
        const default_action = spgw175();
    }
    @hidden table tbl_port_counter31 {
        actions = {
            port_counter31();
        }
        const default_action = port_counter31();
    }
    @hidden table tbl_port_counter34 {
        actions = {
            port_counter34();
        }
        const default_action = port_counter34();
    }
    apply {
        tbl_fabric78.apply();
        if (hdr.gtpu.isValid()) {
            ;
        } else {
            tbl_spgw30.apply();
        }
        if (spgw_normalizer_hasReturned) {
            ;
        } else {
            tbl_spgw31.apply();
            if (hdr.inner_udp.isValid()) {
                tbl_spgw35.apply();
            } else {
                tbl_spgw37.apply();
            }
        }
        if (hdr.packet_out.isValid()) {
            tbl_packetio25.apply();
        }
        if (hasExited) {
            ;
        } else {
            if (hdr.vlan_tag.isValid()) {
                tbl_filtering105.apply();
            }
            if (hdr.mpls.isValid()) {
                ;
            } else {
                tbl_filtering115.apply();
            }
            filtering_ingress_port_vlan.apply();
            filtering_fwd_classifier.apply();
            tbl_act.apply();
            if (hdr.gtpu.isValid()) {
                if (spgw_ingress_s1u_filter_table.apply().hit) {
                    ;
                } else {
                    tbl_spgw149.apply();
                }
                tbl_spgw151.apply();
                tbl_spgw_ingress_gtpu_decap.apply();
            } else if (spgw_ingress_dl_sess_lookup.apply().hit) {
                tbl_spgw154.apply();
            } else {
                tbl_spgw156.apply();
            }
            if (spgw_ingress_hasReturned) {
                ;
            } else {
                tbl_spgw175.apply();
            }
            if (fabric_metadata._skip_forwarding7) {
                ;
            } else if (fabric_metadata._fwd_type9 == 3w0) {
                forwarding_bridging.apply();
            } else if (fabric_metadata._fwd_type9 == 3w1) {
                forwarding_mpls.apply();
            } else if (fabric_metadata._fwd_type9 == 3w2) {
                forwarding_routing_v4.apply();
            }
            acl_acl.apply();
            if (fabric_metadata._skip_next8) {
                ;
            } else {
                next_xconnect.apply();
                next_hashed.apply();
                next_multicast.apply();
                next_next_vlan.apply();
                if (standard_metadata.egress_spec < 9w511) {
                    tbl_port_counter31.apply();
                }
                if (standard_metadata.ingress_port < 9w511) {
                    tbl_port_counter34.apply();
                }
            }
        }
    }
}

control FabricEgress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    bool hasExited_0;
    @name(".nop") action nop_11() {
    }
    @hidden @name("FabricEgress.egress_next.pop_mpls_if_present") action egress_next_pop_mpls_if_present_0() {
        hdr.mpls.setInvalid();
        fabric_metadata._eth_type0 = fabric_metadata._ip_eth_type1;
    }
    @hidden @name("FabricEgress.egress_next.set_mpls") action egress_next_set_mpls_0() {
        hdr.mpls.setValid();
        hdr.mpls.label = fabric_metadata._mpls_label5;
        hdr.mpls.tc = 3w0;
        hdr.mpls.bos = 1w1;
        hdr.mpls.ttl = fabric_metadata._mpls_ttl6;
        fabric_metadata._eth_type0 = 16w0x8847;
    }
    @hidden @name("FabricEgress.egress_next.push_vlan") action egress_next_push_vlan_0() {
        hdr.vlan_tag.setValid();
        hdr.vlan_tag.cfi = fabric_metadata._vlan_cfi4;
        hdr.vlan_tag.pri = fabric_metadata._vlan_pri3;
        hdr.vlan_tag.eth_type = fabric_metadata._eth_type0;
        hdr.vlan_tag.vlan_id = fabric_metadata._vlan_id2;
        hdr.ethernet.eth_type = 16w0x8100;
    }
    @name("FabricEgress.egress_next.egress_vlan_counter") direct_counter(CounterType.packets_and_bytes) egress_next_egress_vlan_counter;
    @name("FabricEgress.egress_next.pop_vlan") action egress_next_pop_vlan_0() {
        hdr.ethernet.eth_type = fabric_metadata._eth_type0;
        hdr.vlan_tag.setInvalid();
        egress_next_egress_vlan_counter.count();
    }
    @name("FabricEgress.egress_next.egress_vlan") table egress_next_egress_vlan {
        key = {
            fabric_metadata._vlan_id2    : exact @name("vlan_id");
            standard_metadata.egress_port: exact @name("eg_port");
        }
        actions = {
            egress_next_pop_vlan_0();
            @defaultonly nop_11();
        }
        const default_action = nop_11();
        counters = egress_next_egress_vlan_counter;
        size = 1024;
    }
    @hidden @name("FabricEgress.spgw_egress.gtpu_encap") action spgw_egress_gtpu_encap_0() {
        hdr.gtpu_ipv4.setValid();
        hdr.gtpu_ipv4.version = 4w4;
        hdr.gtpu_ipv4.ihl = 4w5;
        hdr.gtpu_ipv4.dscp = 6w0;
        hdr.gtpu_ipv4.ecn = 2w0;
        hdr.gtpu_ipv4.total_len = hdr.ipv4.total_len + 16w36;
        hdr.gtpu_ipv4.identification = 16w0x1513;
        hdr.gtpu_ipv4.flags = 3w0;
        hdr.gtpu_ipv4.frag_offset = 13w0;
        hdr.gtpu_ipv4.ttl = 8w64;
        hdr.gtpu_ipv4.protocol = 8w17;
        hdr.gtpu_ipv4.dst_addr = fabric_metadata._spgw_s1u_enb_addr20;
        hdr.gtpu_ipv4.src_addr = fabric_metadata._spgw_s1u_sgw_addr21;
        hdr.gtpu_ipv4.hdr_checksum = 16w0;
        hdr.gtpu_udp.setValid();
        hdr.gtpu_udp.sport = 16w2152;
        hdr.gtpu_udp.dport = 16w2152;
        hdr.gtpu_udp.len = fabric_metadata._spgw_ipv4_len18 + 16w16;
        hdr.gtpu_udp.checksum = 16w0;
        hdr.gtpu.setValid();
        hdr.gtpu.version = 3w0x1;
        hdr.gtpu.pt = 1w0x1;
        hdr.gtpu.spare = 1w0;
        hdr.gtpu.ex_flag = 1w0;
        hdr.gtpu.seq_flag = 1w0;
        hdr.gtpu.npdu_flag = 1w0;
        hdr.gtpu.msgtype = 8w0xff;
        hdr.gtpu.msglen = fabric_metadata._spgw_ipv4_len18;
        hdr.gtpu.teid = fabric_metadata._spgw_teid19;
    }
    @hidden action packetio41() {
        hasExited_0 = true;
    }
    @hidden action act_0() {
        hasExited_0 = false;
    }
    @hidden action packetio47() {
        mark_to_drop(standard_metadata);
    }
    @hidden action packetio49() {
        hdr.packet_in.setValid();
        hdr.packet_in.ingress_port = standard_metadata.ingress_port;
        hasExited_0 = true;
    }
    @hidden action next308() {
        mark_to_drop(standard_metadata);
    }
    @hidden action next327() {
        mark_to_drop(standard_metadata);
    }
    @hidden action next326() {
        hdr.mpls.ttl = hdr.mpls.ttl + 8w255;
    }
    @hidden action next331() {
        mark_to_drop(standard_metadata);
    }
    @hidden action next330() {
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_packetio41 {
        actions = {
            packetio41();
        }
        const default_action = packetio41();
    }
    @hidden table tbl_packetio47 {
        actions = {
            packetio47();
        }
        const default_action = packetio47();
    }
    @hidden table tbl_packetio49 {
        actions = {
            packetio49();
        }
        const default_action = packetio49();
    }
    @hidden table tbl_next308 {
        actions = {
            next308();
        }
        const default_action = next308();
    }
    @hidden table tbl_egress_next_pop_mpls_if_present {
        actions = {
            egress_next_pop_mpls_if_present_0();
        }
        const default_action = egress_next_pop_mpls_if_present_0();
    }
    @hidden table tbl_egress_next_set_mpls {
        actions = {
            egress_next_set_mpls_0();
        }
        const default_action = egress_next_set_mpls_0();
    }
    @hidden table tbl_egress_next_push_vlan {
        actions = {
            egress_next_push_vlan_0();
        }
        const default_action = egress_next_push_vlan_0();
    }
    @hidden table tbl_next326 {
        actions = {
            next326();
        }
        const default_action = next326();
    }
    @hidden table tbl_next327 {
        actions = {
            next327();
        }
        const default_action = next327();
    }
    @hidden table tbl_next330 {
        actions = {
            next330();
        }
        const default_action = next330();
    }
    @hidden table tbl_next331 {
        actions = {
            next331();
        }
        const default_action = next331();
    }
    @hidden table tbl_spgw_egress_gtpu_encap {
        actions = {
            spgw_egress_gtpu_encap_0();
        }
        const default_action = spgw_egress_gtpu_encap_0();
    }
    apply {
        tbl_act_0.apply();
        if (fabric_metadata._is_controller_packet_out12) {
            tbl_packetio41.apply();
        }
        if (hasExited_0) {
            ;
        } else if (standard_metadata.egress_port == 9w255) {
            if (fabric_metadata._is_multicast11 && !fabric_metadata._clone_to_cpu13) {
                tbl_packetio47.apply();
            }
            tbl_packetio49.apply();
        }
        if (hasExited_0) {
            ;
        } else {
            if (fabric_metadata._is_multicast11 && standard_metadata.ingress_port == standard_metadata.egress_port) {
                tbl_next308.apply();
            }
            if (fabric_metadata._mpls_label5 == 20w0) {
                if (hdr.mpls.isValid()) {
                    tbl_egress_next_pop_mpls_if_present.apply();
                }
            } else {
                tbl_egress_next_set_mpls.apply();
            }
            if (egress_next_egress_vlan.apply().hit) {
                ;
            } else if (fabric_metadata._vlan_id2 != 12w4094) {
                tbl_egress_next_push_vlan.apply();
            }
            if (hdr.mpls.isValid()) {
                tbl_next326.apply();
                if (hdr.mpls.ttl == 8w0) {
                    tbl_next327.apply();
                }
            } else if (hdr.ipv4.isValid()) {
                tbl_next330.apply();
                if (hdr.ipv4.ttl == 8w0) {
                    tbl_next331.apply();
                }
            }
            if (fabric_metadata._spgw_direction17 == 2w2) {
                tbl_spgw_egress_gtpu_encap.apply();
            }
        }
    }
}

V1Switch<parsed_headers_t, fabric_metadata_t>(FabricParser(), FabricVerifyChecksum(), FabricIngress(), FabricEgress(), FabricComputeChecksum(), FabricDeparser()) main;
