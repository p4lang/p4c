#include "../p4include/ubpf_filter_model.p4"
#include <core.p4>

#define SYNSENT 1
#define SYNACKED 2
#define ESTABLISHED 3

typedef bit<48>  EthernetAddress;
typedef bit<9> egressSpec_t;

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
    //bit<6>  ctrl;
    bit<1> urgent;
    bit<1> ack;
    bit<1> psh;
    bit<1> rst;
    bit<1> syn;
    bit<1> fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct headers_t {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
    tcp_t            tcp;
}

struct ingress_metadata_t {
    bit<32> nhop_ipv4;
}

struct ConnectionInfo_t {
    bit<32> s;
    bit<32> srv_addr;
}

struct metadata_t {
    ConnectionInfo_t connInfo;
    bit<32> conn_id;
}


parser prs(packet_in packet, out headers_t hdr) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition parse_tcp;
    }
    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
}

control pipe(inout headers_t hdr) {
    metadata_t meta;

    Register<bit<32>, bit<32>>(65536) conn_state;
    Register<bit<32>, bit<32>>(65536) conn_srv_addr;

    action update_conn_state(bit<32> s) {
        conn_state.write(meta.conn_id, s);
    }

    action update_conn_info(bit<32> s, bit<32> addr) {
        conn_state.write(meta.conn_id, s);
        conn_srv_addr.write(meta.conn_id, addr);
    }

    action _drop() {
        mark_to_drop();
    }

    apply {
        //_drop();
        if (hdr.tcp.isValid()) {
            @atomic {
                if (hdr.ipv4.srcAddr < hdr.ipv4.dstAddr) {
                    hash(meta.conn_id, HashAlgorithm.lookup3, { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol, hdr.tcp.srcPort, hdr.tcp.dstPort });
                } else {
                    hash(meta.conn_id, HashAlgorithm.lookup3, { hdr.ipv4.dstAddr, hdr.ipv4.srcAddr, hdr.ipv4.protocol, hdr.tcp.dstPort, hdr.tcp.srcPort });
                }

                meta.connInfo.s = conn_state.read(meta.conn_id);
                meta.connInfo.srv_addr = conn_srv_addr.read(meta.conn_id);
                if (meta.connInfo.s == 0 || meta.connInfo.srv_addr == 0) {
                    if (hdr.tcp.syn == 1 && hdr.tcp.ack == 0) {
                        // It's a SYN
                        update_conn_info(SYNSENT, hdr.ipv4.dstAddr);
                    }
                    //_drop();
                } else if (meta.connInfo.srv_addr == hdr.ipv4.srcAddr) {
                    if (meta.connInfo.s == SYNSENT) {
                        if (hdr.tcp.syn == 1 && hdr.tcp.ack == 1) {
                            // It's a SYN-ACK
                            update_conn_state(SYNACKED);
                        }
                        //_drop();
                    } else if (meta.connInfo.s == SYNACKED) {
                        _drop();
                        return;
                    } else if (meta.connInfo.s == ESTABLISHED) {
                        if (hdr.tcp.fin == 1 && hdr.tcp.ack == 1) {
                            update_conn_info(0, 0);  // clear register entry
                        }
                        //mark_to_pass();
                    }
                } else {
                    if (meta.connInfo.s == SYNSENT) {
                        _drop();
                        return;
                    } else if (meta.connInfo.s == SYNACKED) {
                        if (hdr.tcp.syn == 0 && hdr.tcp.ack == 1) {
                            // It's a ACK
                            update_conn_state(ESTABLISHED);
                            //mark_to_pass();
                        }
                        //_drop();
                    } else if (meta.connInfo.s == ESTABLISHED) {
                        if (hdr.tcp.fin == 1 && hdr.tcp.ack == 1) {
                            update_conn_info(0, 0); // clear register entry
                        }
                        //mark_to_pass();
                    }
                }
            }
        }
    }
}

ubpfFilter(prs(), pipe()) main;