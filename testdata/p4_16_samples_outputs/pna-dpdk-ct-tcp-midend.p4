#include <core.p4>
#include <pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    bit<8>  dir;
    bit<8>  currentState;
    bit<8>  tcpFlagsSet;
    bit<32> hashCode;
    bit<32> replyHashCode;
    bit<32> connHashCode;
    bool    doAddOnMiss;
    bool    updateExpireTimeoutProfile;
    bool    restartExpireTimer;
    bool    acceptPacket;
    bit<8>  timeoutProfileId;
}

struct tcp_ct_table_hit_params_t {
}

Hash<bit<32>>(PNA_HashAlgorithm_t.TARGET_DEFAULT) tcp5TupleHash;
Register<bit<8>, bit<32>>(32w0x10000) tcpCtCurrentStateReg;
Register<bit<8>, bit<32>>(32w0x10000) tcpCtDirReg;
parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ethernet.etherType) {
            16w0x6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
}

struct tuple_1 {
    bit<8>  f0;
    bit<32> f1;
    bit<32> f2;
    bit<16> f3;
    bit<16> f4;
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.key_0") bit<32> key_0;
    @name("MainControlImpl.key_1") bit<32> key_1;
    @name("MainControlImpl.key_3") bit<16> key_3;
    @name("MainControlImpl.key_4") bit<16> key_4;
    @name("MainControlImpl.tmp_2") bit<32> tmp_0;
    @name("MainControlImpl.tmp_4") bit<32> tmp_2;
    @name("MainControlImpl.tmp_7") bit<16> tmp_5;
    @name("MainControlImpl.tmp_9") bit<16> tmp_7;
    @name("MainControlImpl.tmp_10") tuple_0 tmp_8;
    @name("MainControlImpl.hasReturned_0") bool hasReturned;
    @name("MainControlImpl.retval_0") bit<8> retval;
    @name("MainControlImpl.tmp") bit<8> tmp_9;
    @name("MainControlImpl.hasReturned") bool hasReturned_0;
    @name("MainControlImpl.retval") bit<8> retval_0;
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_1() {
        drop_packet();
    }
    @name("MainControlImpl.tcp_ct_syn_sent") action tcp_ct_syn_sent() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x1);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w3;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = true;
    }
    @name("MainControlImpl.tcp_ct_syn_recv") action tcp_ct_syn_recv() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x2);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w2;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_established") action tcp_ct_established() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x3);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w4;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_fin_wait") action tcp_ct_fin_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x4);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w3;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_close_wait") action tcp_ct_close_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x5);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w2;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_last_ack") action tcp_ct_last_ack() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x6);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w1;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_time_wait") action tcp_ct_time_wait() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x7);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w3;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_close") action tcp_ct_close() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x8);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w0;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_syn_sent2") action tcp_ct_syn_sent2() {
        tcpCtCurrentStateReg.write(meta.connHashCode, 8w0x9);
        meta.updateExpireTimeoutProfile = true;
        meta.timeoutProfileId = 8w3;
        meta.restartExpireTimer = true;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_ignore") action tcp_ct_ignore() {
        meta.updateExpireTimeoutProfile = false;
        meta.restartExpireTimer = false;
        meta.doAddOnMiss = false;
    }
    @name("MainControlImpl.tcp_ct_state_table") table tcp_ct_state_table_0 {
        key = {
            retval           : exact @name("meta.dir");
            retval_0         : exact @name("meta.tcpFlagsSet");
            meta.currentState: exact @name("meta.currentState");
        }
        actions = {
            drop();
            tcp_ct_syn_sent();
            tcp_ct_syn_recv();
            tcp_ct_established();
            tcp_ct_fin_wait();
            tcp_ct_close_wait();
            tcp_ct_last_ack();
            tcp_ct_time_wait();
            tcp_ct_close();
            tcp_ct_syn_sent2();
            tcp_ct_ignore();
        }
        const default_action = drop();
    }
    @name("MainControlImpl.tcp_ct_table_hit") action tcp_ct_table_hit() {
        meta.acceptPacket = true;
    }
    @name("MainControlImpl.tcp_ct_table_miss") action tcp_ct_table_miss() {
        meta.acceptPacket = meta.doAddOnMiss;
    }
    @name("MainControlImpl.tcp_ct_table") table tcp_ct_table_0 {
        key = {
            key_0            : exact @name("ipv4_addr_0");
            key_1            : exact @name("ipv4_addr_1");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
            key_3            : exact @name("tcp_port_0");
            key_4            : exact @name("tcp_port_1");
        }
        actions = {
            @tableonly tcp_ct_table_hit();
            @defaultonly tcp_ct_table_miss();
        }
        add_on_miss = true;
        default_idle_timeout_for_data_plane_added_entries = 1;
        idle_timeout_with_auto_delete = true;
        const default_action = tcp_ct_table_miss();
    }
    @name("MainControlImpl.send") action send(@name("port") bit<32> port) {
        send_to_port(port);
    }
    @name("MainControlImpl.ip_packet_fwd") table ip_packet_fwd_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            drop_1();
            send();
        }
        const default_action = drop_1();
        size = 0x1000;
    }
    @hidden action pnadpdkcttcp180() {
        hasReturned = true;
        retval = 8w0x1;
    }
    @hidden action pnadpdkcttcp470() {
        meta.hashCode = tcp5TupleHash.get_hash<tuple_1>((tuple_1){f0 = hdr.ipv4.protocol,f1 = hdr.ipv4.srcAddr,f2 = hdr.ipv4.dstAddr,f3 = hdr.tcp.srcPort,f4 = hdr.tcp.dstPort});
        meta.replyHashCode = tcp5TupleHash.get_hash<tuple_1>((tuple_1){f0 = hdr.ipv4.protocol,f1 = hdr.ipv4.dstAddr,f2 = hdr.ipv4.srcAddr,f3 = hdr.tcp.dstPort,f4 = hdr.tcp.srcPort});
        tmp_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
        tmp_2 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
        tmp_5 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort);
        tmp_7 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort);
        tmp_8.f0 = tmp_0;
        tmp_8.f1 = tmp_2;
        tmp_8.f2 = hdr.ipv4.protocol;
        tmp_8.f3 = tmp_5;
        tmp_8.f4 = tmp_7;
        meta.connHashCode = tcp5TupleHash.get_hash<tuple_0>(tmp_8);
        hasReturned = false;
        tmp_9 = tcpCtDirReg.read(meta.replyHashCode);
    }
    @hidden action pnadpdkcttcp183() {
        retval = 8w0x0;
    }
    @hidden action pnadpdkcttcp153() {
        hasReturned_0 = true;
        retval_0 = 8w0x5;
    }
    @hidden action pnadpdkcttcp492() {
        meta.dir = retval;
        meta.currentState = tcpCtCurrentStateReg.read(meta.connHashCode);
        hasReturned_0 = false;
    }
    @hidden action pnadpdkcttcp159() {
        hasReturned_0 = true;
        retval_0 = 8w0x2;
    }
    @hidden action pnadpdkcttcp161() {
        hasReturned_0 = true;
        retval_0 = 8w0x1;
    }
    @hidden action pnadpdkcttcp167() {
        hasReturned_0 = true;
        retval_0 = 8w0x3;
    }
    @hidden action pnadpdkcttcp172() {
        hasReturned_0 = true;
        retval_0 = 8w0x4;
    }
    @hidden action pnadpdkcttcp175() {
        retval_0 = 8w0x6;
    }
    @hidden action pnadpdkcttcp498() {
        meta.tcpFlagsSet = retval_0;
    }
    @hidden action pnadpdkcttcp416() {
        key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
        key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
        key_3 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort);
        key_4 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort);
    }
    @hidden action pnadpdkcttcp461() {
        meta.doAddOnMiss = false;
        meta.restartExpireTimer = false;
        meta.updateExpireTimeoutProfile = false;
        meta.acceptPacket = false;
    }
    @hidden table tbl_pnadpdkcttcp461 {
        actions = {
            pnadpdkcttcp461();
        }
        const default_action = pnadpdkcttcp461();
    }
    @hidden table tbl_pnadpdkcttcp470 {
        actions = {
            pnadpdkcttcp470();
        }
        const default_action = pnadpdkcttcp470();
    }
    @hidden table tbl_pnadpdkcttcp180 {
        actions = {
            pnadpdkcttcp180();
        }
        const default_action = pnadpdkcttcp180();
    }
    @hidden table tbl_pnadpdkcttcp183 {
        actions = {
            pnadpdkcttcp183();
        }
        const default_action = pnadpdkcttcp183();
    }
    @hidden table tbl_pnadpdkcttcp492 {
        actions = {
            pnadpdkcttcp492();
        }
        const default_action = pnadpdkcttcp492();
    }
    @hidden table tbl_pnadpdkcttcp153 {
        actions = {
            pnadpdkcttcp153();
        }
        const default_action = pnadpdkcttcp153();
    }
    @hidden table tbl_pnadpdkcttcp159 {
        actions = {
            pnadpdkcttcp159();
        }
        const default_action = pnadpdkcttcp159();
    }
    @hidden table tbl_pnadpdkcttcp161 {
        actions = {
            pnadpdkcttcp161();
        }
        const default_action = pnadpdkcttcp161();
    }
    @hidden table tbl_pnadpdkcttcp167 {
        actions = {
            pnadpdkcttcp167();
        }
        const default_action = pnadpdkcttcp167();
    }
    @hidden table tbl_pnadpdkcttcp172 {
        actions = {
            pnadpdkcttcp172();
        }
        const default_action = pnadpdkcttcp172();
    }
    @hidden table tbl_pnadpdkcttcp175 {
        actions = {
            pnadpdkcttcp175();
        }
        const default_action = pnadpdkcttcp175();
    }
    @hidden table tbl_pnadpdkcttcp498 {
        actions = {
            pnadpdkcttcp498();
        }
        const default_action = pnadpdkcttcp498();
    }
    @hidden table tbl_pnadpdkcttcp416 {
        actions = {
            pnadpdkcttcp416();
        }
        const default_action = pnadpdkcttcp416();
    }
    apply {
        tbl_pnadpdkcttcp461.apply();
        if (hdr.tcp.isValid()) {
            tbl_pnadpdkcttcp470.apply();
            if (tmp_9 == 8w0x0) {
                tbl_pnadpdkcttcp180.apply();
            }
            if (hasReturned) {
                ;
            } else {
                tbl_pnadpdkcttcp183.apply();
            }
            tbl_pnadpdkcttcp492.apply();
            if (hdr.tcp.flags & 8w0x4 != 8w0) {
                tbl_pnadpdkcttcp153.apply();
            }
            if (hasReturned_0) {
                ;
            } else if (hdr.tcp.flags & 8w0x2 != 8w0) {
                if (hdr.tcp.flags & 8w0x10 != 8w0) {
                    tbl_pnadpdkcttcp159.apply();
                } else {
                    tbl_pnadpdkcttcp161.apply();
                }
            }
            if (hasReturned_0) {
                ;
            } else if (hdr.tcp.flags & 8w0x1 != 8w0) {
                tbl_pnadpdkcttcp167.apply();
            }
            if (hasReturned_0) {
                ;
            } else if (hdr.tcp.flags & 8w0x10 != 8w0) {
                tbl_pnadpdkcttcp172.apply();
            }
            if (hasReturned_0) {
                ;
            } else {
                tbl_pnadpdkcttcp175.apply();
            }
            tbl_pnadpdkcttcp498.apply();
            tcp_ct_state_table_0.apply();
            tbl_pnadpdkcttcp416.apply();
            tcp_ct_table_0.apply();
        }
        if (meta.acceptPacket) {
            ip_packet_fwd_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkcttcp519() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
        pkt.emit<tcp_t>(hdr.tcp);
    }
    @hidden table tbl_pnadpdkcttcp519 {
        actions = {
            pnadpdkcttcp519();
        }
        const default_action = pnadpdkcttcp519();
    }
    apply {
        tbl_pnadpdkcttcp519.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
