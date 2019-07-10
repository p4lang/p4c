#include <core.p4>
#include <v1model.p4>

typedef bit<3> fwd_type_t;
typedef bit<32> next_id_t;
typedef bit<20> mpls_label_t;
typedef bit<9> port_num_t;
typedef bit<48> mac_addr_t;
typedef bit<16> mcast_group_id_t;
typedef bit<12> vlan_id_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> l4_port_t;
typedef bit<2> direction_t;
typedef bit<1> pcc_gate_status_t;
typedef bit<32> sdf_rule_id_t;
typedef bit<32> pcc_rule_id_t;
const ipv4_addr_t S1U_SGW_PREFIX = 2348810240;
const bit<16> ETHERTYPE_QINQ = 0x88a8;
const bit<16> ETHERTYPE_QINQ_NON_STD = 0x9100;
const bit<16> ETHERTYPE_VLAN = 0x8100;
const bit<16> ETHERTYPE_MPLS = 0x8847;
const bit<16> ETHERTYPE_MPLS_MULTICAST = 0x8848;
const bit<16> ETHERTYPE_IPV4 = 0x800;
const bit<16> ETHERTYPE_IPV6 = 0x86dd;
const bit<16> ETHERTYPE_ARP = 0x806;
const bit<8> PROTO_ICMP = 1;
const bit<8> PROTO_TCP = 6;
const bit<8> PROTO_UDP = 17;
const bit<8> PROTO_ICMPV6 = 58;
const bit<4> IPV4_MIN_IHL = 5;
const fwd_type_t FWD_BRIDGING = 0;
const fwd_type_t FWD_MPLS = 1;
const fwd_type_t FWD_IPV4_UNICAST = 2;
const fwd_type_t FWD_IPV4_MULTICAST = 3;
const fwd_type_t FWD_IPV6_UNICAST = 4;
const fwd_type_t FWD_IPV6_MULTICAST = 5;
const fwd_type_t FWD_UNKNOWN = 7;
const vlan_id_t DEFAULT_VLAN_ID = 12w4094;
const bit<8> DEFAULT_MPLS_TTL = 64;
const bit<8> DEFAULT_IPV4_TTL = 64;
const sdf_rule_id_t DEFAULT_SDF_RULE_ID = 0;
const pcc_rule_id_t DEFAULT_PCC_RULE_ID = 0;
const direction_t SPGW_DIR_UNKNOWN = 2w0;
const direction_t SPGW_DIR_UPLINK = 2w1;
const direction_t SPGW_DIR_DOWNLINK = 2w2;
const pcc_gate_status_t PCC_GATE_OPEN = 1w0;
const pcc_gate_status_t PCC_GATE_CLOSED = 1w1;
const bit<6> INT_DSCP = 0x1;
const bit<8> INT_HEADER_LEN_WORDS = 4;
const bit<16> INT_HEADER_LEN_BYTES = 16;
const bit<8> CPU_MIRROR_SESSION_ID = 250;
const bit<32> REPORT_MIRROR_SESSION_ID = 500;
const bit<4> NPROTO_ETHERNET = 0;
const bit<4> NPROTO_TELEMETRY_DROP_HEADER = 1;
const bit<4> NPROTO_TELEMETRY_SWITCH_LOCAL_HEADER = 2;
const bit<6> HW_ID = 1;
const bit<8> REPORT_FIXED_HEADER_LEN = 12;
const bit<8> DROP_REPORT_HEADER_LEN = 12;
const bit<8> LOCAL_REPORT_HEADER_LEN = 16;
const bit<8> ETH_HEADER_LEN = 14;
const bit<8> IPV4_MIN_HEAD_LEN = 20;
const bit<8> UDP_HEADER_LEN = 8;
action nop() {
    NoAction();
}
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
    port_num_t ingress_port;
    bit<7>     _pad;
}

@controller_header("packet_out") header packet_out_header_t {
    port_num_t egress_port;
    bit<7>     _pad;
}

header ethernet_t {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    eth_type;
}

header vlan_tag_t {
    bit<3>    pri;
    bit<1>    cfi;
    vlan_id_t vlan_id;
    bit<16>   eth_type;
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
    direction_t direction;
    bit<16>     ipv4_len;
    bit<32>     teid;
    bit<32>     s1u_enb_addr;
    bit<32>     s1u_sgw_addr;
}

struct fabric_metadata_t {
    bit<16>      eth_type;
    bit<16>      ip_eth_type;
    vlan_id_t    vlan_id;
    bit<3>       vlan_pri;
    bit<1>       vlan_cfi;
    mpls_label_t mpls_label;
    bit<8>       mpls_ttl;
    bool         skip_forwarding;
    bool         skip_next;
    fwd_type_t   fwd_type;
    next_id_t    next_id;
    bool         is_multicast;
    bool         is_controller_packet_out;
    bool         clone_to_cpu;
    bit<8>       ip_proto;
    bit<16>      l4_sport;
    bit<16>      l4_dport;
    spgw_meta_t  spgw;
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

control Filtering(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    direct_counter(CounterType.packets_and_bytes) ingress_port_vlan_counter;
    action deny() {
        fabric_metadata.skip_forwarding = true;
        fabric_metadata.skip_next = true;
        ingress_port_vlan_counter.count();
    }
    action permit() {
        ingress_port_vlan_counter.count();
    }
    action permit_with_internal_vlan(vlan_id_t vlan_id) {
        fabric_metadata.vlan_id = vlan_id;
        ingress_port_vlan_counter.count();
    }
    table ingress_port_vlan {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port") ;
            hdr.vlan_tag.isValid()        : exact @name("vlan_is_valid") ;
            hdr.vlan_tag.vlan_id          : ternary @name("vlan_id") ;
        }
        actions = {
            deny();
            permit();
            permit_with_internal_vlan();
        }
        const default_action = deny();
        counters = ingress_port_vlan_counter;
        size = 1024;
    }
    direct_counter(CounterType.packets_and_bytes) fwd_classifier_counter;
    action set_forwarding_type(fwd_type_t fwd_type) {
        fabric_metadata.fwd_type = fwd_type;
        fwd_classifier_counter.count();
    }
    table fwd_classifier {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port") ;
            hdr.ethernet.dst_addr         : ternary @name("eth_dst") ;
            fabric_metadata.eth_type      : exact @name("eth_type") ;
        }
        actions = {
            set_forwarding_type;
        }
        const default_action = set_forwarding_type(FWD_BRIDGING);
        counters = fwd_classifier_counter;
        size = 1024;
    }
    apply {
        if (hdr.vlan_tag.isValid()) {
            fabric_metadata.eth_type = hdr.vlan_tag.eth_type;
            fabric_metadata.vlan_id = hdr.vlan_tag.vlan_id;
            fabric_metadata.vlan_pri = hdr.vlan_tag.pri;
            fabric_metadata.vlan_cfi = hdr.vlan_tag.cfi;
        }
        if (!hdr.mpls.isValid()) {
            fabric_metadata.mpls_ttl = DEFAULT_MPLS_TTL + 1;
        }
        ingress_port_vlan.apply();
        fwd_classifier.apply();
    }
}

control Forwarding(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action set_next_id(next_id_t next_id) {
        fabric_metadata.next_id = next_id;
    }
    direct_counter(CounterType.packets_and_bytes) bridging_counter;
    action set_next_id_bridging(next_id_t next_id) {
        set_next_id(next_id);
        bridging_counter.count();
    }
    table bridging {
        key = {
            fabric_metadata.vlan_id: exact @name("vlan_id") ;
            hdr.ethernet.dst_addr  : ternary @name("eth_dst") ;
        }
        actions = {
            set_next_id_bridging;
            @defaultonly nop;
        }
        const default_action = nop();
        counters = bridging_counter;
        size = 1024;
    }
    direct_counter(CounterType.packets_and_bytes) mpls_counter;
    action pop_mpls_and_next(next_id_t next_id) {
        fabric_metadata.mpls_label = 0;
        set_next_id(next_id);
        mpls_counter.count();
    }
    table mpls {
        key = {
            fabric_metadata.mpls_label: exact @name("mpls_label") ;
        }
        actions = {
            pop_mpls_and_next;
            @defaultonly nop;
        }
        const default_action = nop();
        counters = mpls_counter;
        size = 1024;
    }
    direct_counter(CounterType.packets_and_bytes) routing_v4_counter;
    action set_next_id_routing_v4(next_id_t next_id) {
        set_next_id(next_id);
        routing_v4_counter.count();
    }
    action nop_routing_v4() {
        routing_v4_counter.count();
    }
    table routing_v4 {
        key = {
            hdr.ipv4.dst_addr: lpm @name("ipv4_dst") ;
        }
        actions = {
            set_next_id_routing_v4;
            nop_routing_v4;
            @defaultonly nop;
        }
        const default_action = nop();
        counters = routing_v4_counter;
        size = 1024;
    }
    apply {
        if (fabric_metadata.fwd_type == FWD_BRIDGING) {
            bridging.apply();
        } else if (fabric_metadata.fwd_type == FWD_MPLS) {
            mpls.apply();
        } else if (fabric_metadata.fwd_type == FWD_IPV4_UNICAST) {
            routing_v4.apply();
        }
    }
}

control Acl(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    direct_counter(CounterType.packets_and_bytes) acl_counter;
    action set_next_id_acl(next_id_t next_id) {
        fabric_metadata.next_id = next_id;
        acl_counter.count();
    }
    action punt_to_cpu() {
        standard_metadata.egress_spec = 255;
        fabric_metadata.skip_next = true;
        acl_counter.count();
    }
    action clone_to_cpu() {
        fabric_metadata.clone_to_cpu = true;
        acl_counter.count();
    }
    action drop() {
        mark_to_drop(standard_metadata);
        fabric_metadata.skip_next = true;
        acl_counter.count();
    }
    action nop_acl() {
        acl_counter.count();
    }
    table acl {
        key = {
            standard_metadata.ingress_port: ternary @name("ig_port") ;
            fabric_metadata.ip_proto      : ternary @name("ip_proto") ;
            fabric_metadata.l4_sport      : ternary @name("l4_sport") ;
            fabric_metadata.l4_dport      : ternary @name("l4_dport") ;
            hdr.ethernet.dst_addr         : ternary @name("eth_src") ;
            hdr.ethernet.src_addr         : ternary @name("eth_dst") ;
            hdr.vlan_tag.vlan_id          : ternary @name("vlan_id") ;
            fabric_metadata.eth_type      : ternary @name("eth_type") ;
            hdr.ipv4.src_addr             : ternary @name("ipv4_src") ;
            hdr.ipv4.dst_addr             : ternary @name("ipv4_dst") ;
            hdr.icmp.icmp_type            : ternary @name("icmp_type") ;
            hdr.icmp.icmp_code            : ternary @name("icmp_code") ;
        }
        actions = {
            set_next_id_acl;
            punt_to_cpu;
            clone_to_cpu;
            drop;
            nop_acl;
        }
        const default_action = nop_acl();
        size = 1024;
        counters = acl_counter;
    }
    apply {
        acl.apply();
    }
}

control Next(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action output(port_num_t port_num) {
        standard_metadata.egress_spec = port_num;
    }
    @hidden action rewrite_smac(mac_addr_t smac) {
        hdr.ethernet.src_addr = smac;
    }
    @hidden action rewrite_dmac(mac_addr_t dmac) {
        hdr.ethernet.dst_addr = dmac;
    }
    @hidden action set_mpls_label(mpls_label_t label) {
        fabric_metadata.mpls_label = label;
    }
    @hidden action routing(port_num_t port_num, mac_addr_t smac, mac_addr_t dmac) {
        rewrite_smac(smac);
        rewrite_dmac(dmac);
        output(port_num);
    }
    @hidden action mpls_routing(port_num_t port_num, mac_addr_t smac, mac_addr_t dmac, mpls_label_t label) {
        set_mpls_label(label);
        routing(port_num, smac, dmac);
    }
    direct_counter(CounterType.packets_and_bytes) next_vlan_counter;
    action set_vlan(vlan_id_t vlan_id) {
        fabric_metadata.vlan_id = vlan_id;
        next_vlan_counter.count();
    }
    table next_vlan {
        key = {
            fabric_metadata.next_id: exact @name("next_id") ;
        }
        actions = {
            set_vlan;
            @defaultonly nop;
        }
        const default_action = nop();
        counters = next_vlan_counter;
        size = 1024;
    }
    direct_counter(CounterType.packets_and_bytes) xconnect_counter;
    action output_xconnect(port_num_t port_num) {
        output(port_num);
        xconnect_counter.count();
    }
    action set_next_id_xconnect(next_id_t next_id) {
        fabric_metadata.next_id = next_id;
        xconnect_counter.count();
    }
    table xconnect {
        key = {
            standard_metadata.ingress_port: exact @name("ig_port") ;
            fabric_metadata.next_id       : exact @name("next_id") ;
        }
        actions = {
            output_xconnect;
            set_next_id_xconnect;
            @defaultonly nop;
        }
        counters = xconnect_counter;
        const default_action = nop();
        size = 1024;
    }
    @max_group_size(16) action_selector(HashAlgorithm.crc16, 32w1024, 32w16) hashed_selector;
    direct_counter(CounterType.packets_and_bytes) hashed_counter;
    action output_hashed(port_num_t port_num) {
        output(port_num);
        hashed_counter.count();
    }
    action routing_hashed(port_num_t port_num, mac_addr_t smac, mac_addr_t dmac) {
        routing(port_num, smac, dmac);
        hashed_counter.count();
    }
    action mpls_routing_hashed(port_num_t port_num, mac_addr_t smac, mac_addr_t dmac, mpls_label_t label) {
        mpls_routing(port_num, smac, dmac, label);
        hashed_counter.count();
    }
    table hashed {
        key = {
            fabric_metadata.next_id : exact @name("next_id") ;
            hdr.ipv4.dst_addr       : selector;
            hdr.ipv4.src_addr       : selector;
            fabric_metadata.ip_proto: selector;
            fabric_metadata.l4_sport: selector;
            fabric_metadata.l4_dport: selector;
        }
        actions = {
            output_hashed;
            routing_hashed;
            mpls_routing_hashed;
            @defaultonly nop;
        }
        implementation = hashed_selector;
        counters = hashed_counter;
        const default_action = nop();
        size = 1024;
    }
    direct_counter(CounterType.packets_and_bytes) multicast_counter;
    action set_mcast_group_id(mcast_group_id_t group_id) {
        standard_metadata.mcast_grp = group_id;
        fabric_metadata.is_multicast = true;
        multicast_counter.count();
    }
    table multicast {
        key = {
            fabric_metadata.next_id: exact @name("next_id") ;
        }
        actions = {
            set_mcast_group_id;
            @defaultonly nop;
        }
        counters = multicast_counter;
        const default_action = nop();
        size = 1024;
    }
    apply {
        xconnect.apply();
        hashed.apply();
        multicast.apply();
        next_vlan.apply();
    }
}

control EgressNextControl(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action pop_mpls_if_present() {
        hdr.mpls.setInvalid();
        fabric_metadata.eth_type = fabric_metadata.ip_eth_type;
    }
    @hidden action set_mpls() {
        hdr.mpls.setValid();
        hdr.mpls.label = fabric_metadata.mpls_label;
        hdr.mpls.tc = 3w0;
        hdr.mpls.bos = 1w1;
        hdr.mpls.ttl = fabric_metadata.mpls_ttl;
        fabric_metadata.eth_type = ETHERTYPE_MPLS;
    }
    @hidden action push_vlan() {
        hdr.vlan_tag.setValid();
        hdr.vlan_tag.cfi = fabric_metadata.vlan_cfi;
        hdr.vlan_tag.pri = fabric_metadata.vlan_pri;
        hdr.vlan_tag.eth_type = fabric_metadata.eth_type;
        hdr.vlan_tag.vlan_id = fabric_metadata.vlan_id;
        hdr.ethernet.eth_type = ETHERTYPE_VLAN;
    }
    direct_counter(CounterType.packets_and_bytes) egress_vlan_counter;
    action pop_vlan() {
        hdr.ethernet.eth_type = fabric_metadata.eth_type;
        hdr.vlan_tag.setInvalid();
        egress_vlan_counter.count();
    }
    table egress_vlan {
        key = {
            fabric_metadata.vlan_id      : exact @name("vlan_id") ;
            standard_metadata.egress_port: exact @name("eg_port") ;
        }
        actions = {
            pop_vlan;
            @defaultonly nop;
        }
        const default_action = nop();
        counters = egress_vlan_counter;
        size = 1024;
    }
    apply {
        if (fabric_metadata.is_multicast == true && standard_metadata.ingress_port == standard_metadata.egress_port) {
            mark_to_drop(standard_metadata);
        }
        if (fabric_metadata.mpls_label == 0) {
            if (hdr.mpls.isValid()) {
                pop_mpls_if_present();
            }
        } else {
            set_mpls();
        }
        if (!egress_vlan.apply().hit) {
            if (fabric_metadata.vlan_id != DEFAULT_VLAN_ID) {
                push_vlan();
            }
        }
        if (hdr.mpls.isValid()) {
            hdr.mpls.ttl = hdr.mpls.ttl - 1;
            if (hdr.mpls.ttl == 0) {
                mark_to_drop(standard_metadata);
            }
        } else {
            if (hdr.ipv4.isValid()) {
                hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
                if (hdr.ipv4.ttl == 0) {
                    mark_to_drop(standard_metadata);
                }
            }
        }
    }
}

control PacketIoIngress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.packet_out.isValid()) {
            standard_metadata.egress_spec = hdr.packet_out.egress_port;
            hdr.packet_out.setInvalid();
            fabric_metadata.is_controller_packet_out = true;
            exit;
        }
    }
}

control PacketIoEgress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (fabric_metadata.is_controller_packet_out == true) {
            exit;
        }
        if (standard_metadata.egress_port == 255) {
            if (fabric_metadata.is_multicast == true && fabric_metadata.clone_to_cpu == false) {
                mark_to_drop(standard_metadata);
            }
            hdr.packet_in.setValid();
            hdr.packet_in.ingress_port = standard_metadata.ingress_port;
            exit;
        }
    }
}

control spgw_normalizer(in bool is_gtpu_encapped, out ipv4_t gtpu_ipv4, out udp_t gtpu_udp, inout ipv4_t ipv4, inout udp_t udp, in ipv4_t inner_ipv4, in udp_t inner_udp) {
    apply {
        if (!is_gtpu_encapped) {
            return;
        }
        gtpu_ipv4 = ipv4;
        ipv4 = inner_ipv4;
        gtpu_udp = udp;
        if (inner_udp.isValid()) {
            udp = inner_udp;
        } else {
            udp.setInvalid();
        }
    }
}

control spgw_ingress(inout ipv4_t gtpu_ipv4, inout udp_t gtpu_udp, inout gtpu_t gtpu, inout ipv4_t ipv4, inout udp_t udp, inout fabric_metadata_t fabric_meta, inout standard_metadata_t standard_metadata) {
    direct_counter(CounterType.packets_and_bytes) ue_counter;
    @hidden action gtpu_decap() {
        gtpu_ipv4.setInvalid();
        gtpu_udp.setInvalid();
        gtpu.setInvalid();
    }
    action set_dl_sess_info(bit<32> teid, bit<32> s1u_enb_addr, bit<32> s1u_sgw_addr) {
        fabric_meta.spgw.teid = teid;
        fabric_meta.spgw.s1u_enb_addr = s1u_enb_addr;
        fabric_meta.spgw.s1u_sgw_addr = s1u_sgw_addr;
        ue_counter.count();
    }
    table dl_sess_lookup {
        key = {
            ipv4.dst_addr: exact @name("ipv4_dst") ;
        }
        actions = {
            set_dl_sess_info();
            @defaultonly nop();
        }
        const default_action = nop();
        counters = ue_counter;
    }
    table s1u_filter_table {
        key = {
            gtpu_ipv4.dst_addr: exact @name("gtp_ipv4_dst") ;
        }
        actions = {
            nop();
        }
        const default_action = nop();
    }
    apply {
        if (gtpu.isValid()) {
            if (!s1u_filter_table.apply().hit) {
                mark_to_drop(standard_metadata);
            }
            fabric_meta.spgw.direction = SPGW_DIR_UPLINK;
            gtpu_decap();
        } else if (dl_sess_lookup.apply().hit) {
            fabric_meta.spgw.direction = SPGW_DIR_DOWNLINK;
        } else {
            fabric_meta.spgw.direction = SPGW_DIR_UNKNOWN;
            return;
        }
        fabric_meta.spgw.ipv4_len = ipv4.total_len;
    }
}

control spgw_egress(in ipv4_t ipv4, inout ipv4_t gtpu_ipv4, inout udp_t gtpu_udp, inout gtpu_t gtpu, in fabric_metadata_t fabric_meta, in standard_metadata_t std_meta) {
    @hidden action gtpu_encap() {
        gtpu_ipv4.setValid();
        gtpu_ipv4.version = 4;
        gtpu_ipv4.ihl = IPV4_MIN_IHL;
        gtpu_ipv4.dscp = 0;
        gtpu_ipv4.ecn = 0;
        gtpu_ipv4.total_len = ipv4.total_len + (20 + 8 + 8);
        gtpu_ipv4.identification = 0x1513;
        gtpu_ipv4.flags = 0;
        gtpu_ipv4.frag_offset = 0;
        gtpu_ipv4.ttl = DEFAULT_IPV4_TTL;
        gtpu_ipv4.protocol = PROTO_UDP;
        gtpu_ipv4.dst_addr = fabric_meta.spgw.s1u_enb_addr;
        gtpu_ipv4.src_addr = fabric_meta.spgw.s1u_sgw_addr;
        gtpu_ipv4.hdr_checksum = 0;
        gtpu_udp.setValid();
        gtpu_udp.sport = 2152;
        gtpu_udp.dport = 2152;
        gtpu_udp.len = fabric_meta.spgw.ipv4_len + (8 + 8);
        gtpu_udp.checksum = 0;
        gtpu.setValid();
        gtpu.version = 0x1;
        gtpu.pt = 0x1;
        gtpu.spare = 0;
        gtpu.ex_flag = 0;
        gtpu.seq_flag = 0;
        gtpu.npdu_flag = 0;
        gtpu.msgtype = 0xff;
        gtpu.msglen = fabric_meta.spgw.ipv4_len;
        gtpu.teid = fabric_meta.spgw.teid;
    }
    apply {
        if (fabric_meta.spgw.direction == SPGW_DIR_DOWNLINK) {
            gtpu_encap();
        }
    }
}

control update_gtpu_checksum(inout ipv4_t gtpu_ipv4, inout udp_t gtpu_udp, in gtpu_t gtpu, in ipv4_t ipv4, in udp_t udp) {
    apply {
        update_checksum(gtpu_ipv4.isValid(), { gtpu_ipv4.version, gtpu_ipv4.ihl, gtpu_ipv4.dscp, gtpu_ipv4.ecn, gtpu_ipv4.total_len, gtpu_ipv4.identification, gtpu_ipv4.flags, gtpu_ipv4.frag_offset, gtpu_ipv4.ttl, gtpu_ipv4.protocol, gtpu_ipv4.src_addr, gtpu_ipv4.dst_addr }, gtpu_ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

control FabricComputeChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
        update_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.hdr_checksum, HashAlgorithm.csum16);
        update_gtpu_checksum.apply(hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu, hdr.ipv4, hdr.udp);
    }
}

control FabricVerifyChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
        verify_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

parser FabricParser(packet_in packet, out parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    bit<6> last_ipv4_dscp = 0;
    state start {
        transition select(standard_metadata.ingress_port) {
            255: parse_packet_out;
            default: parse_ethernet;
        }
    }
    state parse_packet_out {
        packet.extract(hdr.packet_out);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        fabric_metadata.eth_type = hdr.ethernet.eth_type;
        fabric_metadata.vlan_id = DEFAULT_VLAN_ID;
        transition select(hdr.ethernet.eth_type) {
            ETHERTYPE_VLAN: parse_vlan_tag;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_vlan_tag {
        packet.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.eth_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_VLAN: parse_inner_vlan_tag;
            default: accept;
        }
    }
    state parse_inner_vlan_tag {
        packet.extract(hdr.inner_vlan_tag);
        transition select(hdr.inner_vlan_tag.eth_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_MPLS: parse_mpls;
            default: accept;
        }
    }
    state parse_mpls {
        packet.extract(hdr.mpls);
        fabric_metadata.mpls_label = hdr.mpls.label;
        fabric_metadata.mpls_ttl = hdr.mpls.ttl;
        transition select(packet.lookahead<bit<4>>()) {
            4: parse_ipv4;
            default: parse_ethernet;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        fabric_metadata.ip_proto = hdr.ipv4.protocol;
        fabric_metadata.ip_eth_type = ETHERTYPE_IPV4;
        last_ipv4_dscp = hdr.ipv4.dscp;
        transition select(hdr.ipv4.protocol) {
            PROTO_TCP: parse_tcp;
            PROTO_UDP: parse_udp;
            PROTO_ICMP: parse_icmp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract(hdr.tcp);
        fabric_metadata.l4_sport = hdr.tcp.sport;
        fabric_metadata.l4_dport = hdr.tcp.dport;
        transition accept;
    }
    state parse_udp {
        packet.extract(hdr.udp);
        fabric_metadata.l4_sport = hdr.udp.sport;
        fabric_metadata.l4_dport = hdr.udp.dport;
        transition select(hdr.udp.dport) {
            2152: parse_gtpu;
            default: accept;
        }
    }
    state parse_icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }
    state parse_gtpu {
        transition select(hdr.ipv4.dst_addr[31:32 - 8]) {
            S1U_SGW_PREFIX[31:32 - 8]: do_parse_gtpu;
            default: accept;
        }
    }
    state do_parse_gtpu {
        packet.extract(hdr.gtpu);
        transition parse_inner_ipv4;
    }
    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        last_ipv4_dscp = hdr.inner_ipv4.dscp;
        transition select(hdr.inner_ipv4.protocol) {
            PROTO_TCP: parse_tcp;
            PROTO_UDP: parse_inner_udp;
            PROTO_ICMP: parse_icmp;
            default: accept;
        }
    }
    state parse_inner_udp {
        packet.extract(hdr.inner_udp);
        fabric_metadata.l4_sport = hdr.inner_udp.sport;
        fabric_metadata.l4_dport = hdr.inner_udp.dport;
        transition accept;
    }
}

control FabricDeparser(packet_out packet, in parsed_headers_t hdr) {
    apply {
        packet.emit(hdr.packet_in);
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag);
        packet.emit(hdr.inner_vlan_tag);
        packet.emit(hdr.mpls);
        packet.emit(hdr.gtpu_ipv4);
        packet.emit(hdr.gtpu_udp);
        packet.emit(hdr.gtpu);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
        packet.emit(hdr.icmp);
    }
}

control PortCountersControl(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    counter(511, CounterType.packets_and_bytes) egress_port_counter;
    counter(511, CounterType.packets_and_bytes) ingress_port_counter;
    apply {
        if (standard_metadata.egress_spec < 511) {
            egress_port_counter.count((bit<32>)standard_metadata.egress_spec);
        }
        if (standard_metadata.ingress_port < 511) {
            ingress_port_counter.count((bit<32>)standard_metadata.ingress_port);
        }
    }
}

control FabricIngress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    PacketIoIngress() pkt_io_ingress;
    Filtering() filtering;
    Forwarding() forwarding;
    Acl() acl;
    Next() next;
    PortCountersControl() port_counters_control;
    apply {
        spgw_normalizer.apply(hdr.gtpu.isValid(), hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.ipv4, hdr.udp, hdr.inner_ipv4, hdr.inner_udp);
        pkt_io_ingress.apply(hdr, fabric_metadata, standard_metadata);
        filtering.apply(hdr, fabric_metadata, standard_metadata);
        spgw_ingress.apply(hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu, hdr.ipv4, hdr.udp, fabric_metadata, standard_metadata);
        if (fabric_metadata.skip_forwarding == false) {
            forwarding.apply(hdr, fabric_metadata, standard_metadata);
        }
        acl.apply(hdr, fabric_metadata, standard_metadata);
        if (fabric_metadata.skip_next == false) {
            next.apply(hdr, fabric_metadata, standard_metadata);
            port_counters_control.apply(hdr, fabric_metadata, standard_metadata);
        }
    }
}

control FabricEgress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    PacketIoEgress() pkt_io_egress;
    EgressNextControl() egress_next;
    apply {
        pkt_io_egress.apply(hdr, fabric_metadata, standard_metadata);
        egress_next.apply(hdr, fabric_metadata, standard_metadata);
        spgw_egress.apply(hdr.ipv4, hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu, fabric_metadata, standard_metadata);
    }
}

V1Switch(FabricParser(), FabricVerifyChecksum(), FabricIngress(), FabricEgress(), FabricComputeChecksum(), FabricDeparser()) main;

