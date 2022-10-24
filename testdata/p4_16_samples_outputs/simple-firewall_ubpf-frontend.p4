#include <core.p4>
#include <ubpf_model.p4>

typedef bit<48> EthernetAddress;
header Ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    ConnectionInfo_t connInfo;
    bit<32>          conn_id;
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

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.s") bit<32> s_0;
    @name("pipe.s") bit<32> s_1;
    @name("pipe.s") bit<32> s_7;
    @name("pipe.addr") bit<32> addr_0;
    @name("pipe.s") bit<32> s_8;
    @name("pipe.addr") bit<32> addr_3;
    @name("pipe.s") bit<32> s_9;
    @name("pipe.addr") bit<32> addr_4;
    @name("pipe.conn_state") Register<bit<32>, bit<32>>(32w65536) conn_state_0;
    @name("pipe.conn_srv_addr") Register<bit<32>, bit<32>>(32w65536) conn_srv_addr_0;
    @name("pipe.update_conn_state") action update_conn_state() {
        s_0 = 32w2;
        conn_state_0.write(meta.conn_id, s_0);
    }
    @name("pipe.update_conn_state") action update_conn_state_1() {
        s_1 = 32w3;
        conn_state_0.write(meta.conn_id, s_1);
    }
    @name("pipe.update_conn_info") action update_conn_info() {
        s_7 = 32w1;
        addr_0 = headers.ipv4.dstAddr;
        conn_state_0.write(meta.conn_id, s_7);
        conn_srv_addr_0.write(meta.conn_id, addr_0);
    }
    @name("pipe.update_conn_info") action update_conn_info_1() {
        s_8 = 32w0;
        addr_3 = 32w0;
        conn_state_0.write(meta.conn_id, s_8);
        conn_srv_addr_0.write(meta.conn_id, addr_3);
    }
    @name("pipe.update_conn_info") action update_conn_info_2() {
        s_9 = 32w0;
        addr_4 = 32w0;
        conn_state_0.write(meta.conn_id, s_9);
        conn_srv_addr_0.write(meta.conn_id, addr_4);
    }
    @name("pipe._drop") action _drop() {
        mark_to_drop();
    }
    @name("pipe._drop") action _drop_1() {
        mark_to_drop();
    }
    apply {
        if (headers.tcp.isValid()) {
            if (headers.ipv4.srcAddr < headers.ipv4.dstAddr) {
                hash<tuple<bit<32>, bit<32>>>(meta.conn_id, HashAlgorithm.lookup3, { headers.ipv4.srcAddr, headers.ipv4.dstAddr });
            } else {
                hash<tuple<bit<32>, bit<32>>>(meta.conn_id, HashAlgorithm.lookup3, { headers.ipv4.dstAddr, headers.ipv4.srcAddr });
            }
            meta.connInfo.s = conn_state_0.read(meta.conn_id);
            meta.connInfo.srv_addr = conn_srv_addr_0.read(meta.conn_id);
            if (meta.connInfo.s == 32w0 || meta.connInfo.srv_addr == 32w0) {
                if (headers.tcp.syn == 1w1 && headers.tcp.ack == 1w0) {
                    update_conn_info();
                }
            } else if (meta.connInfo.srv_addr == headers.ipv4.srcAddr) {
                if (meta.connInfo.s == 32w1) {
                    if (headers.tcp.syn == 1w1 && headers.tcp.ack == 1w1) {
                        update_conn_state();
                    }
                } else if (meta.connInfo.s == 32w2) {
                    _drop();
                } else if (meta.connInfo.s == 32w3) {
                    if (headers.tcp.fin == 1w1 && headers.tcp.ack == 1w1) {
                        update_conn_info_1();
                    }
                }
            } else if (meta.connInfo.s == 32w1) {
                _drop_1();
            } else if (meta.connInfo.s == 32w2) {
                if (headers.tcp.syn == 1w0 && headers.tcp.ack == 1w1) {
                    update_conn_state_1();
                }
            } else if (meta.connInfo.s == 32w3) {
                if (headers.tcp.fin == 1w1 && headers.tcp.ack == 1w1) {
                    update_conn_info_2();
                }
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_t>(headers.ethernet);
        packet.emit<Ipv4_t>(headers.ipv4);
        packet.emit<Tcp_t>(headers.tcp);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
