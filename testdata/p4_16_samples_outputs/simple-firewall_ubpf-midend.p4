#include <core.p4>
#include <ubpf_model.p4>

header Ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header Ipv4_t {
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

header Tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<1>  urgent;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct Headers_t {
    Ethernet_t ethernet;
    Ipv4_t     ipv4;
    Tcp_t      tcp;
}

struct ConnectionInfo_t {
    bit<32> s;
    bit<32> srv_addr;
}

struct metadata {
    bit<32> _connInfo_s0;
    bit<32> _connInfo_srv_addr1;
    bit<32> _conn_id2;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_t>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        p.extract<Ipv4_t>(headers.ipv4);
        p.extract<Tcp_t>(headers.tcp);
        transition accept;
    }
}

struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.conn_state") Register<bit<32>, bit<32>>(32w65536) conn_state_0;
    @name("pipe.conn_srv_addr") Register<bit<32>, bit<32>>(32w65536) conn_srv_addr_0;
    @name("pipe.update_conn_state") action update_conn_state() {
        conn_state_0.write(meta._conn_id2, 32w2);
    }
    @name("pipe.update_conn_state") action update_conn_state_1() {
        conn_state_0.write(meta._conn_id2, 32w3);
    }
    @name("pipe.update_conn_info") action update_conn_info() {
        conn_state_0.write(meta._conn_id2, 32w1);
        conn_srv_addr_0.write(meta._conn_id2, headers.ipv4.dstAddr);
    }
    @name("pipe.update_conn_info") action update_conn_info_1() {
        conn_state_0.write(meta._conn_id2, 32w0);
        conn_srv_addr_0.write(meta._conn_id2, 32w0);
    }
    @name("pipe.update_conn_info") action update_conn_info_2() {
        conn_state_0.write(meta._conn_id2, 32w0);
        conn_srv_addr_0.write(meta._conn_id2, 32w0);
    }
    @name("pipe._drop") action _drop() {
        mark_to_drop();
    }
    @name("pipe._drop") action _drop_1() {
        mark_to_drop();
    }
    @hidden action simplefirewall_ubpf125() {
        hash<tuple_0>(meta._conn_id2, HashAlgorithm.lookup3, (tuple_0){f0 = headers.ipv4.srcAddr,f1 = headers.ipv4.dstAddr});
    }
    @hidden action simplefirewall_ubpf127() {
        hash<tuple_0>(meta._conn_id2, HashAlgorithm.lookup3, (tuple_0){f0 = headers.ipv4.dstAddr,f1 = headers.ipv4.srcAddr});
    }
    @hidden action simplefirewall_ubpf130() {
        meta._connInfo_s0 = conn_state_0.read(meta._conn_id2);
        meta._connInfo_srv_addr1 = conn_srv_addr_0.read(meta._conn_id2);
    }
    @hidden table tbl_simplefirewall_ubpf125 {
        actions = {
            simplefirewall_ubpf125();
        }
        const default_action = simplefirewall_ubpf125();
    }
    @hidden table tbl_simplefirewall_ubpf127 {
        actions = {
            simplefirewall_ubpf127();
        }
        const default_action = simplefirewall_ubpf127();
    }
    @hidden table tbl_simplefirewall_ubpf130 {
        actions = {
            simplefirewall_ubpf130();
        }
        const default_action = simplefirewall_ubpf130();
    }
    @hidden table tbl_update_conn_info {
        actions = {
            update_conn_info();
        }
        const default_action = update_conn_info();
    }
    @hidden table tbl_update_conn_state {
        actions = {
            update_conn_state();
        }
        const default_action = update_conn_state();
    }
    @hidden table tbl__drop {
        actions = {
            _drop();
        }
        const default_action = _drop();
    }
    @hidden table tbl_update_conn_info_0 {
        actions = {
            update_conn_info_1();
        }
        const default_action = update_conn_info_1();
    }
    @hidden table tbl__drop_0 {
        actions = {
            _drop_1();
        }
        const default_action = _drop_1();
    }
    @hidden table tbl_update_conn_state_0 {
        actions = {
            update_conn_state_1();
        }
        const default_action = update_conn_state_1();
    }
    @hidden table tbl_update_conn_info_1 {
        actions = {
            update_conn_info_2();
        }
        const default_action = update_conn_info_2();
    }
    apply {
        if (headers.tcp.isValid()) {
            if (headers.ipv4.srcAddr < headers.ipv4.dstAddr) {
                tbl_simplefirewall_ubpf125.apply();
            } else {
                tbl_simplefirewall_ubpf127.apply();
            }
            tbl_simplefirewall_ubpf130.apply();
            if (meta._connInfo_s0 == 32w0 || meta._connInfo_srv_addr1 == 32w0) {
                if (headers.tcp.syn == 1w1 && headers.tcp.ack == 1w0) {
                    tbl_update_conn_info.apply();
                }
            } else if (meta._connInfo_srv_addr1 == headers.ipv4.srcAddr) {
                if (meta._connInfo_s0 == 32w1) {
                    if (headers.tcp.syn == 1w1 && headers.tcp.ack == 1w1) {
                        tbl_update_conn_state.apply();
                    }
                } else if (meta._connInfo_s0 == 32w2) {
                    tbl__drop.apply();
                } else if (meta._connInfo_s0 == 32w3) {
                    if (headers.tcp.fin == 1w1 && headers.tcp.ack == 1w1) {
                        tbl_update_conn_info_0.apply();
                    }
                }
            } else if (meta._connInfo_s0 == 32w1) {
                tbl__drop_0.apply();
            } else if (meta._connInfo_s0 == 32w2) {
                if (headers.tcp.syn == 1w0 && headers.tcp.ack == 1w1) {
                    tbl_update_conn_state_0.apply();
                }
            } else if (meta._connInfo_s0 == 32w3) {
                if (headers.tcp.fin == 1w1 && headers.tcp.ack == 1w1) {
                    tbl_update_conn_info_1.apply();
                }
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action simplefirewall_ubpf172() {
        packet.emit<Ethernet_t>(headers.ethernet);
        packet.emit<Ipv4_t>(headers.ipv4);
        packet.emit<Tcp_t>(headers.tcp);
    }
    @hidden table tbl_simplefirewall_ubpf172 {
        actions = {
            simplefirewall_ubpf172();
        }
        const default_action = simplefirewall_ubpf172();
    }
    apply {
        tbl_simplefirewall_ubpf172.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
