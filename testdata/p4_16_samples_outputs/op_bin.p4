#include <core.p4>
#include <ebpf_model.p4>

typedef bit<48> EthernetAddress;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header IPv6_h {
    bit<32>  ip_version_traffic_class_and_flow_label;
    bit<16>  payload_length;
    bit<8>   protocol;
    bit<8>   hop_limit;
    bit<128> src_address;
    bit<128> dst_address;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv6_h     ipv6;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            0x86dd: ipv6;
            default: reject;
        }
    }
    state ipv6 {
        p.extract(headers.ipv6);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool xout) {
    action set_flowlabel(bit<20> label) {
        headers.ipv6.ip_version_traffic_class_and_flow_label[31:12] = label;
    }
    table filter_tbl {
        key = {
            headers.ipv6.src_address: exact;
        }
        actions = {
            set_flowlabel;
        }
        const entries = {
                        8w0x20 ++ 8w0x2 ++ 8w0x4 ++ 8w0x20 ++ 8w0x3 ++ 8w0x80 ++ 8w0xde ++ 8w0xad ++ 8w0xbe ++ 8w0xef ++ 8w0xf0 ++ 8w0xd ++ 8w0xd ++ 8w0x9 ++ 8w0 ++ 8w0x1 : set_flowlabel(52);
        }
        implementation = hash_table(8);
    }
    apply {
        xout = true;
        filter_tbl.apply();
        if (headers.ipv6.isValid() && (headers.ethernet.etherType == 0x86dd || headers.ipv6.hop_limit == 255)) {
            headers.ipv6.protocol = 17;
        }
        if (headers.ethernet.etherType == 0x800) {
            headers.ipv6.protocol = 10;
        }
    }
}

ebpfFilter(prs(), pipe()) main;
