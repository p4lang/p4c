#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress_t;
const bit<8> ETHERNET_HEADER_LEN = 14;
header ethernet_t {
    EthernetAddress_t dstAddr;
    EthernetAddress_t srcAddr;
    bit<16>           etherType;
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
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

struct main_metadata_t {
    bit<8>                dir;
    bit<8>                currentState;
    bit<8>                tcpFlagsSet;
    bit<32>               hashCode;
    bit<32>               replyHashCode;
    bit<32>               connHashCode;
    bool                  doAddOnMiss;
    bool                  updateExpireTimeoutProfile;
    bool                  restartExpireTimer;
    bool                  acceptPacket;
    ExpireTimeProfileId_t timeoutProfileId;
}

struct tcp_ct_table_hit_params_t {
}

Hash<bit<32>>(PNA_HashAlgorithm_t.TARGET_DEFAULT) tcp5TupleHash;
const bit<32> TCP_CT_MAX_FLOW_CNT = 0x10000;
Register<bit<8>, bit<32>>(TCP_CT_MAX_FLOW_CNT) tcpCtCurrentStateReg;
Register<bit<8>, bit<32>>(TCP_CT_MAX_FLOW_CNT) tcpCtDirReg;
const bit<8> CT_DIR_ORIGINAL = 0x0;
const bit<8> CT_DIR_REPLY = 0x1;
const bit<8> TCP_URG_MASK = 0x20;
const bit<8> TCP_ACK_MASK = 0x10;
const bit<8> TCP_PSH_MASK = 0x8;
const bit<8> TCP_RST_MASK = 0x4;
const bit<8> TCP_SYN_MASK = 0x2;
const bit<8> TCP_FIN_MASK = 0x1;
const bit<8> TCP_SYN_SET = 0x1;
const bit<8> TCP_SYNACK_SET = 0x2;
const bit<8> TCP_FIN_SET = 0x3;
const bit<8> TCP_ACK_SET = 0x4;
const bit<8> TCP_RST_SET = 0x5;
const bit<8> TCP_NONE_SET = 0x6;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_SYN_SENT = (ExpireTimeProfileId_t)3;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_SYN_RECV = (ExpireTimeProfileId_t)2;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_ESTABLISHED = (ExpireTimeProfileId_t)4;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_FIN_WAIT = (ExpireTimeProfileId_t)3;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_CLOSE_WAIT = (ExpireTimeProfileId_t)2;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_LAST_ACK = (ExpireTimeProfileId_t)1;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_TIME_WAIT = (ExpireTimeProfileId_t)3;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_CLOSE = (ExpireTimeProfileId_t)0;
const ExpireTimeProfileId_t TCP_CT_TIMEOUT_SYN_SENT2 = (ExpireTimeProfileId_t)3;
const bit<8> TCP_CT_NONE = 0x0;
const bit<8> TCP_CT_SYN_SENT = 0x1;
const bit<8> TCP_CT_SYN_RECV = 0x2;
const bit<8> TCP_CT_ESTABLISHED = 0x3;
const bit<8> TCP_CT_FIN_WAIT = 0x4;
const bit<8> TCP_CT_CLOSE_WAIT = 0x5;
const bit<8> TCP_CT_LAST_ACK = 0x6;
const bit<8> TCP_CT_TIME_WAIT = 0x7;
const bit<8> TCP_CT_CLOSE = 0x8;
const bit<8> TCP_CT_SYN_SENT2 = 0x9;
const bit<8> TCP_CT_LISTEN = 0x9;
const bit<8> TCP_CT_MAX = 0xa;
const bit<8> TCP_CT_IGNORE = 0xb;
const bit<8> TCP_CT_RETRANS = 0xc;
const bit<8> TCP_CT_UNACK = 0xd;
const bit<8> TCP_CT_TIMEOUT_MAX = 0xe;
bit<8> getTcpFlagSetInfo(in bit<8> tcp_flags) {
    if (tcp_flags & TCP_RST_MASK != 0) {
        return TCP_RST_SET;
    }
    if (tcp_flags & TCP_SYN_MASK != 0) {
        if (tcp_flags & TCP_ACK_MASK != 0) {
            return TCP_SYNACK_SET;
        } else {
            return TCP_SYN_SET;
        }
    }
    if (tcp_flags & TCP_FIN_MASK != 0) {
        return TCP_FIN_SET;
    }
    if (tcp_flags & TCP_ACK_MASK != 0) {
        return TCP_ACK_SET;
    }
    return TCP_NONE_SET;
}
bit<8> getTcpDirection(in bit<32> hash, in bit<32> replyHash) {
    if (tcpCtDirReg.read(replyHash) == CT_DIR_ORIGINAL) {
        return CT_DIR_REPLY;
    }
    return CT_DIR_ORIGINAL;
}
parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ethernet.etherType) {
            0x6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    bool add_succeeded;
    action drop() {
        drop_packet();
    }
    action tcp_ct_syn_sent() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_SYN_SENT);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_SYN_SENT;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = true;
    }
    action tcp_ct_syn_recv() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_SYN_RECV);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_SYN_RECV;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_established() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_ESTABLISHED);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_ESTABLISHED;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_fin_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_FIN_WAIT);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_FIN_WAIT;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_close_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_CLOSE_WAIT);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_CLOSE_WAIT;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_last_ack() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_LAST_ACK);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_LAST_ACK;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_time_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_TIME_WAIT);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_TIME_WAIT;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_close() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_CLOSE);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_CLOSE;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_syn_sent2() {
        tcpCtCurrentStateReg.write(meta.connHashCode, TCP_CT_SYN_SENT2);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = (ExpireTimeProfileId_t)TCP_CT_TIMEOUT_SYN_SENT2;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    action tcp_ct_ignore() {
        meta.updateExpireTimeoutProfile = false;
        meta.restartExpireTimer = false;
        meta.doAddOnMiss = false;
    }
    table tcp_ct_state_table {
        key = {
            meta.dir         : exact;
            meta.tcpFlagsSet : exact;
            meta.currentState: exact;
        }
        actions = {
            drop;
            tcp_ct_syn_sent;
            tcp_ct_syn_recv;
            tcp_ct_established;
            tcp_ct_fin_wait;
            tcp_ct_close_wait;
            tcp_ct_last_ack;
            tcp_ct_time_wait;
            tcp_ct_close;
            tcp_ct_syn_sent2;
            tcp_ct_ignore;
        }
        const default_action = drop;
    }
    action tcp_ct_table_hit() {
        meta.acceptPacket = true;
    }
    action tcp_ct_table_miss() {
        if (meta.doAddOnMiss) {
            meta.acceptPacket = true;
        } else {
            meta.acceptPacket = false;
        }
    }
    table tcp_ct_table {
        key = {
            SelectByDirection(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr): exact @name("ipv4_addr_0");
            SelectByDirection(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr): exact @name("ipv4_addr_1");
            hdr.ipv4.protocol                                                    : exact;
            SelectByDirection(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort)  : exact @name("tcp_port_0");
            SelectByDirection(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort)  : exact @name("tcp_port_1");
        }
        actions = {
            @tableonly tcp_ct_table_hit;
            @defaultonly tcp_ct_table_miss;
        }
        add_on_miss = true;
        default_idle_timeout_for_data_plane_added_entries = 1;
        idle_timeout_with_auto_delete = true;
        const default_action = tcp_ct_table_miss;
    }
    action send(PortId_t port) {
        send_to_port(port);
    }
    table ip_packet_fwd {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            drop;
            send;
        }
        const default_action = drop;
        size = 0x1000;
    }
    apply {
        meta.doAddOnMiss = false;
        meta.restartExpireTimer = false;
        meta.updateExpireTimeoutProfile = false;
        meta.acceptPacket = false;
        if (hdr.tcp.isValid()) {
            meta.hashCode = tcp5TupleHash.get_hash({ hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.tcp.srcPort, hdr.tcp.dstPort });
            meta.replyHashCode = tcp5TupleHash.get_hash({ hdr.ipv4.protocol, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr, hdr.tcp.dstPort, hdr.tcp.srcPort });
            meta.connHashCode = tcp5TupleHash.get_hash({ SelectByDirection(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr), SelectByDirection(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr), hdr.ipv4.protocol, SelectByDirection(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort), SelectByDirection(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort) });
            meta.dir = getTcpDirection(meta.hashCode, meta.replyHashCode);
            meta.currentState = tcpCtCurrentStateReg.read(meta.connHashCode);
            meta.tcpFlagsSet = getTcpFlagSetInfo(hdr.tcp.flags);
            tcp_ct_state_table.apply();
            tcp_ct_table.apply();
        }
        if (meta.acceptPacket) {
            ip_packet_fwd.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.tcp);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;