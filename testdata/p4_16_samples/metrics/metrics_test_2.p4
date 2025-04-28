#include <core.p4>
#include <v1model.p4>

header Ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  tos;
    bit<16> length;
    bit<16> id;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> checksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header IPv6_t {
    bit<4>  version;
    bit<8>  trafficClass;
    bit<20> flowLabel;
    bit<16> payloadLength;
    bit<8>  nextHeader;
    bit<8>  hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header ARP_t {
    bit<16> htype;
    bit<16> ptype;
    bit<8>  hlen;
    bit<8>  plen;
    bit<16> operation;
    bit<48> senderHwAddr;
    bit<32> senderProtoAddr;
    bit<48> targetHwAddr;
    bit<32> targetProtoAddr;
}

header TCP_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNum;
    bit<32> ackNum;
    bit<4>  dataOffset;
    bit<6>  flags;
    bit<6>  reserved;
    bit<16> windowSize;
    bit<16> checksum;
    bit<16> urgentPointer;
}

header UDP_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length;
    bit<16> checksum;
}

header ICMP_t {
    bit<8> type;
    bit<8> code;
    bit<16> checksum;
    bit<16> identifier;
    bit<16> sequenceNum;
}

header h_stack {
    bit<9> i1;
    bit<7> i2;
}

struct headers {
    Ethernet_t ethernet;
    IPv4_t ipv4;
    IPv6_t ipv6;
    ARP_t arp;
    TCP_t tcp;
    UDP_t udp;
    ICMP_t icmp;
    h_stack[2] h;
}

struct metadata {
    bit<9>  ingress_port;
    bit<8>  priority;
}

error {
    Error1,
    Error2,
    Error3
}

parser MyParser(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        pkt.extract(hdr.ethernet);
        hdr.ethernet.setValid();
        hdr.h[1].setValid();
        hdr.ethernet.srcAddr = 0x001122334455;
        bit<16> bytes_parsed = 10;
        bytes_parsed = bytes_parsed + 10;
        if(bytes_parsed == 20){
            hdr.ipv4.setInvalid();
        }

        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            0x86DD: parse_ipv6;
            0x0806: parse_arp;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        hdr.ipv4.ttl = 64;
        verify(hdr.ipv4.ttl == 5, error.Error1);
        bit<16> bytes_parsed = 10;
        bytes_parsed = bytes_parsed + 10;

        pkt.extract(hdr.h[1]);
        if (hdr.h[1].i2 == 0x00) {
            hdr.ipv4.setInvalid();
        }
        
        if(bytes_parsed == 20){
            hdr.ipv4.setInvalid();
        }

        transition select(hdr.ipv4.protocol) {
            0x06: parse_tcp;
            0x11: parse_udp;
            0x01: parse_icmp;
            default: accept;
        }
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        hdr.ipv6.setValid();
        hdr.ipv6.hopLimit = 32;
        verify(hdr.ipv6.hopLimit > 2, error.Error2);
        hdr.ipv6.setInvalid();
        transition accept;
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        hdr.arp.setValid();
        hdr.arp.operation = 2;
        transition accept;
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        hdr.tcp.setValid();
        hdr.tcp.flags = 0x02;
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        hdr.udp.length = hdr.udp.length + 8;
        transition accept;
    }

    state parse_icmp {
        pkt.extract(hdr.icmp);
        hdr.icmp.type = 0;
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
control MyComputeChecksum(inout headers hdr, inout metadata meta) { apply { } }

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action forward(bit<9> port) {
        standard_metadata.egress_spec = port;
        bit<9> score = port * 2 + hdr.h[0].i1;
        if (score > 5) {
            mark_to_drop(standard_metadata);
        }
    }

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_priority(bit<8> value) {
        meta.priority = value;
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
            hdr.ipv4.protocol: exact;
        }
        actions = {
            forward;
            drop;
        }
        size = 1024;
        default_action = drop();
    }

    apply {
        if (hdr.ipv4.isValid()) {
            if (hdr.ipv4.dstAddr == 0x0A000001) {
                set_priority(255);
            } else if (hdr.ipv4.ttl < 32) {
                set_priority(1);
            } else {
                set_priority(10);
            }

            ipv4_lpm.apply();

            switch (meta.priority) {
                255: {
                    forward(1);
                }
                1: {
                    forward(2);
                }
                10: {
                    forward(3);
                }
                default: {
                    drop();
                }
            }
        } else {
            drop();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action clear_h0() {
        hdr.h[0].i1 = 0;
        hdr.h[0].i2 = 0;
    }

    action set_ttl(bit<8> new_ttl) {
        hdr.ipv4.ttl = new_ttl;
    }

    action update_ttl_and_log(bit<8> new_ttl) {
        set_ttl(new_ttl); // Will be inlined     
        clear_h0(); // Will be inlined   
    }

    action set_priority(bit<8> val, bit<9> port) {
        meta.priority = val;
        standard_metadata.egress_spec = port;
        update_ttl_and_log(32); // Will be inlined
        clear_h0(); // Will be inlined   
    }

    apply {
        hdr.h[1].i2 = hdr.h[1].i2 + 1;

        if (meta.priority == 255) {
            set_priority(1, 2);                               
        } else if (meta.priority > 10) {
            set_priority(2, 3);
        } else {
            set_priority(3, 4);
        }
    }
}

control MyDeparser(packet_out pkt, in headers hdr) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.h[0]);
        pkt.emit(hdr.h[1]);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

