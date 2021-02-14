/*
Copyright 2021-present MNK Labs & Consulting, LLC.

https://mnkcg.com

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

#include <ebpf_model.p4>

typedef bit<48> EthernetAddress;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

header IPv6_h {
    bit<32>     ip_version_traffic_class_and_flow_label;
    bit<16>     payload_length;
    bit<8>      protocol;
    bit<8>      hop_limit;
    bit<128>    src_address;
    bit<128>    dst_address;
}

struct Headers_t
{
    Ethernet_h ethernet;
    IPv6_h     ipv6;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            0x86DD   : ipv6;
            default : reject;
        }
    }

    state ipv6 {
        p.extract(headers.ipv6);
        transition accept;
    }

}

control pipe(inout Headers_t headers, out bool xout) {
    // UBPF does not support const entries, so using hard-coded label below.
    action set_flowlabel(bit<20> label) {
     	headers.ipv6.ip_version_traffic_class_and_flow_label[31:12] = label;
    }

    table filter_tbl {
        key = {
            headers.ipv6.src_address : exact;
        }
        actions = {
            set_flowlabel;
        }
	const entries = {
	    {(8w0x20++8w0x02++8w0x04++8w0x20++8w0x03++8w0x80++8w0xDE++8w0xAD++8w0xBE++8w0xEF++8w0xF0++8w0x0D++8w0x0D++8w0x09++8w0++8w0x01)} : set_flowlabel(52);
	}
	implementation = hash_table(8);
    }

    apply {
        xout = true;
        filter_tbl.apply();

        // If-statement below tests if appropriate parenthesis are
        // added because, if not, gcc compling fails with emitted C code.
        if (headers.ipv6.isValid() && ((headers.ethernet.etherType == 0x86dd)
	    || (headers.ipv6.hop_limit == 255)))
            headers.ipv6.protocol = 17;
        // If-statement below tests if duplicate parenthesis
        // are not added in emitted C code causing gcc compiling failure.
        if (headers.ethernet.etherType == 0x0800)
            headers.ipv6.protocol = 10;
    }
}

ebpfFilter(prs(), pipe()) main;
