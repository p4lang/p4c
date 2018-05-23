/*
Copyright 2017 Barefoot Networks, Inc.

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

#include <core.p4>
#include "../psa.p4"

typedef bit<48>  EthernetAddress;

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

struct fwd_metadata_t {

}

struct telemetry_metadata_t {
    bit<8> reason;
}

struct metadata {
    fwd_metadata_t fwd_metadata;
    telemetry_metadata_t telemetry_md;
    clone_metadata_t cloned_header;
}

struct clone_metadata_t {
    bit<8> clone_tag;
    telemetry_metadata_t telemetry_md;
}

struct headers {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
}

parser CommonParser(packet_in buffer,
                    out headers parsed_hdr,
                    inout metadata user_meta)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract(parsed_hdr.ipv4);
        transition accept;
    }
}

parser CloneParser(packet_in buffer,
                   in psa_ingress_parser_input_metadata_t istd,
                   inout clone_metadata_t clone_meta) {
     state start {
         transition select(buffer.lookahead<bit<8>>()) {
            1 : parse_clone_meta;
            default: reject;
         }
     }
     state parse_clone_meta {
     	 buffer.extract(clone_meta);
	 transition accept;
     }
}

parser IngressParserImpl(packet_in buffer,
                         out headers parsed_hdr,
                         inout metadata user_meta,
                         in psa_ingress_parser_input_metadata_t istd)
{
    CommonParser() p;
    CloneParser() cp;

    state start {
        transition select(istd.packet_path) {
           PSA_PacketPath_t.CLONE: parse_clone_header; // FIXME: CLONE_I2E or CLONE_E2E
           PSA_PacketPath_t.NORMAL: parse_ethernet;
        }
    }

    state parse_clone_header {
        cp.apply(buffer, istd, user_meta.clone_header);
	transition accept;
    }

    state parse_ethernet {
        p.apply(buffer, parsed_hdr, user_meta);
	transition accept;
    }
}

// clone a packet to CPU.
control ingress(inout headers hdr,
                inout metadata user_meta,
                PacketReplicationEngine pre,
                in  psa_ingress_input_metadata_t  istd,
                out psa_ingress_output_metadata_t ostd)
{
    action mirror_on_drop(bit<8> reason, PortId_t port) {
	ostd.clone = 1;
	ostd.clone_port = port;
        ostd.drop = 1;
 	user_meta.telemetry_md.reason = reason;
    }

    table system_acl() {
        key = {
            acl_metadata.acl_deny : ternary;
        }
        actions = { mirror_on_drop; }
    }

    apply {
        system_acl.apply();
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers parsed_hdr,
                        inout metadata user_meta,
                        in psa_egress_parser_input_metadata_t istd)
{
    CommonParser() p;

    state start {
        p.apply(buffer, parsed_hdr, user_meta);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               BufferingQueueingEngine bqe,
               in  psa_egress_input_metadata_t  istd,
               out psa_egress_output_metadata_t ostd)
{
    apply { }
}

control DeparserImpl(packet_out packet, inout headers hdr) {
    apply {
        packet.emit(hdr.eth);
        packet.emit(hdr.ipv4);
    }
}

control IngressDeparserImpl(packet_out packet,
    clone_out clone,
    inout headers hdr,
    in userMetadata meta,
    in psa_ingress_output_metadata_t istd) {
    DeparserImpl() common_deparser;
    apply {
        // user is responsible for constructing the clone header,
        // it may include a user-defined tag to distinguish different
        // clone headers.
        clone_metadata_t clone_md;
        clone_md.tag = 8w1;
        clone_md.telemetry_md = meta.telemetry_md;
        if (istd.clone) {
            clone.add_metadata(clone_md);
        }
        common_deparser.apply(packet, hdr);
    }
}

control EgressDeparserImpl(packet_out packet,
    clone_out clone,
    inout headers hdr,
    in userMetadata meta,
    in psa_egress_output_metadata_t istd,
    in psa_egress_deparser_input_metadata_t edstd)
{
    DeparserImpl() common_deparser;
    apply {
        if (istd.clone) {
            clone.add_metadata({hdr.ipv4.srcAddr, hdr.ipv4.dstAddr});
        }
        common_deparser.apply(packet, hdr);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
