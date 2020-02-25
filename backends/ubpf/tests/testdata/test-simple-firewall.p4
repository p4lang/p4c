/*
Copyright 2019 Orange

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <ubpf_model.p4>
#include <core.p4>

const bit<32> SYNSENT = 1;
const bit<32> SYNACKED = 2;
const bit<32> ESTABLISHED = 3;

typedef bit<48>  EthernetAddress;
typedef bit<9> egressSpec_t;

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

struct Headers_t {
    Ethernet_t       ethernet;
    Ipv4_t           ipv4;
    Tcp_t            tcp;
}

struct ConnectionInfo_t {
    bit<32> s;
    bit<32> srv_addr;
}

struct metadata {
    ConnectionInfo_t connInfo;
    bit<32> conn_id;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        p.extract(headers.ipv4);
        transition parse_tcp;
    }
    state parse_tcp {
        p.extract(headers.tcp);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {

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
        if (headers.tcp.isValid()) {
            if (headers.ipv4.srcAddr < headers.ipv4.dstAddr) {
                hash(meta.conn_id, HashAlgorithm.lookup3, { headers.ipv4.srcAddr, headers.ipv4.dstAddr });
            } else {
                hash(meta.conn_id, HashAlgorithm.lookup3, { headers.ipv4.dstAddr, headers.ipv4.srcAddr });
            }

            meta.connInfo.s = conn_state.read(meta.conn_id);
            meta.connInfo.srv_addr = conn_srv_addr.read(meta.conn_id);
            if (meta.connInfo.s == 0 || meta.connInfo.srv_addr == 0) {
                if (headers.tcp.syn == 1 && headers.tcp.ack == 0) {
                    // It's a SYN
                    update_conn_info(SYNSENT, headers.ipv4.dstAddr);
                }
            } else if (meta.connInfo.srv_addr == headers.ipv4.srcAddr) {
                if (meta.connInfo.s == SYNSENT) {
                    if (headers.tcp.syn == 1 && headers.tcp.ack == 1) {
                        // It's a SYN-ACK
                        update_conn_state(SYNACKED);
                    }
                } else if (meta.connInfo.s == SYNACKED) {
                    _drop();
                    return;
                } else if (meta.connInfo.s == ESTABLISHED) {
                    if (headers.tcp.fin == 1 && headers.tcp.ack == 1) {
                        update_conn_info(0, 0);  // clear register entry
                    }
                }
            } else {
                if (meta.connInfo.s == SYNSENT) {
                    _drop();
                    return;
                } else if (meta.connInfo.s == SYNACKED) {
                    if (headers.tcp.syn == 0 && headers.tcp.ack == 1) {
                        // It's a ACK
                        update_conn_state(ESTABLISHED);
                    }
                } else if (meta.connInfo.s == ESTABLISHED) {
                    if (headers.tcp.fin == 1 && headers.tcp.ack == 1) {
                        update_conn_info(0, 0); // clear register entry
                    }
                }
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
        packet.emit(headers.tcp);
    }
}

ubpf(prs(), pipe(), dprs()) main;