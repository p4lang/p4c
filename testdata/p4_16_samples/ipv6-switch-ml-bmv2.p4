/*
* Copyright 2019, MNK Consulting
* http://mnkcg.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*
*    How to test this P4 code:
* $ ./p4c-bm2-ss --std p4-16 ../testdata/p4_16_samples/ipv6-switch-ml-bmv2.p4 -o tmp.json
*
*/

#include <v1model.p4>
#include "ml-headers.p4"

const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_REPLICATION   = 5;
#define IS_REPLICATED(std_meta) (std_meta.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_REPLICATION)

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV6: ipv6;
            default: accept;
        }
    }
    state ipv6 {
        packet.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            PROTO_UDP: parse_udp;
            PROTO_ICMP6: icmp6;
            default: accept;
        }
    }
    state icmp6 {
        packet.extract(hdr.icmp6);
        transition accept;
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition parse_ml;
    }
    state parse_ml {
        packet.extract(hdr.ml);
	transition parse_ml_vector;
    }
    state parse_ml_vector {
        packet.extract(hdr.vector.next);
	transition select(hdr.vector.last.e) {
            _: accept;
        }
    }

}

// Our switch table comprises of IPv6 addresses vs. egress_port.
// This is the table we setup here.
control ingress(inout headers hdr, inout metadata_t meta,
                inout standard_metadata_t standard_metadata) {

    aggregator_t[S] pool;
    count_t[S]      count;

    action set_mcast_grp(bit<16> mcast_grp, bit<9> port) {
        standard_metadata.mcast_grp = mcast_grp;
	meta.egress_port = port;
    }
    table ipv6_tbl {
	key = {
            (hdr.ipv6.dstAddr[127:120] == 8w0xff) : exact @name("mcast_key");
	}
	actions = {set_mcast_grp;}
    }
    /* TODO: The paper does not mention what IP (unicast or multicast is a
     * worker update sent to the switch. Once it's known, this table should
     * be activated.
    table ipv6_rx {
	key = {
            (hdr.ipv6.srcAddr[127:120] == 8w0xff) : exact @name("rx_key");
	}
	actions = {set_mcast_grp;}
    }
    */
    apply {
        if (hdr.ipv6.isValid()) {
	    ipv6_tbl.apply();
        }
        hdr.vector[0].e = pool[hdr.ml.idx].val;
    }
}

control egress(inout headers hdr, inout metadata_t meta,
               inout standard_metadata_t standard_metadata) {
    apply {}
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {}
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {}
}

V1Switch(MyParser(), MyVerifyChecksum(), ingress(), egress(),
MyComputeChecksum(), MyDeparser()) main;
