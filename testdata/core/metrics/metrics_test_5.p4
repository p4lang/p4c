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

struct headers {
    Ethernet_t ethernet;
    IPv4_t ipv4;
    IPv6_t ipv6;
}

struct metadata {
    bit<9>  ingress_port;
    bit<8>  priority;
    bit<16> var1;
    bit<32> var2;
    bit<8>  var3;
    bit<8>  unused1;
    bit<16> unused2;
}

// Useless
enum useless_enum_t {
    VALUE_A,
    VALUE_B,
    VALUE_C
}

// Useless
void useless_function1() {
    bit<8> temp = 1 + 1;
}

// Useless
void useless_function2(bit<8> param) {
    bit<8> temp = param + 1;
}

bit<9> used_function(inout bit<9> param){
    if(param > 2){
        param = param + 50 * 2;
    }
    else{
        param = param + 20 * 5;
        }
    return param;
}

// Useless
action unused_action(){
}

parser MyParser(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        pkt.extract(hdr.ethernet);
        hdr.ethernet.setValid();
        hdr.ethernet.srcAddr = 0x001122334455;

        meta.var1 = 42;
        meta.var2 = 1000;
        meta.var3 = 5;

        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            0x86DD: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        hdr.ipv4.setValid();
        hdr.ipv4.ttl = 64;
        transition select(hdr.ipv4.protocol) {
            default: accept;
        }
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        hdr.ipv6.setValid();
        hdr.ipv6.hopLimit = 32;
        hdr.ipv6.setInvalid();
        transition accept;
    }

    // Useless
    state unused_state2 {
        bit<32> temp = 10;
        bit<32> local_var2 = ~temp * 5;
        bit<32> local_var3 = local_var2 + 100;
        transition accept;
    }
}


control MyVerifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
control MyComputeChecksum(inout headers hdr, inout metadata meta) { apply { } }

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action forward(bit<9> port) {
        standard_metadata.egress_spec = port;
    }

    action set_ttl(bit<8> new_ttl) {
            hdr.ipv4.ttl = new_ttl;
    }

    action drop() {
        mark_to_drop(standard_metadata);
        set_ttl(2);
    }


    action useless_action() { // Useless
        bit<8> x = +1;
        bit<8> y = -x;
        bit<8> z = ~y;
        return;  // Useless
    }

    apply {
        {
        bit<32> temp = 10; // Useless
        }
        if (hdr.ipv4.isValid()) {
            bit<9> temp_2 = 8;
            if (meta.var1 == 0) { }// Useless
            else {}                // Useless              
            forward(used_function(temp_2));

            if (hdr.ipv4.ihl == 0){ }// Useless

        } else if (hdr.ipv6.isValid()) {
            drop();
        } else {
            if (hdr.ipv6.version == 8) { }  // Useless
            drop();
        }

        return;  // Useless
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action clear() {
        hdr.ipv4.version = 0;
    }

    apply {
        hdr.ipv4.ttl = hdr.ipv4.ttl + 1;
        clear();
    }
}

control MyDeparser(packet_out pkt, in headers hdr) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv6);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

