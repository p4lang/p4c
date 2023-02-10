/*
Copyright 2022-present Orange

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
#include <psa.p4>
#include "common_headers.p4"

struct headers {
    ethernet_t ethernet;
    ipv6_t     ipv6;
}


parser IngressParserImpl(packet_in buffer,
                         out headers parsed_hdr,
                         inout empty_t meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_t resubmit_meta,
                         in empty_t recirculate_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x86DD: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv6 {
        buffer.extract(parsed_hdr.ipv6);
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers parsed_hdr,
                        inout empty_t meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_t normal_meta,
                        in empty_t clone_i2e_meta,
                        in empty_t clone_e2e_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x86DD: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv6 {
        buffer.extract(parsed_hdr.ipv6);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout empty_t meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    action set_dst(bit<128> addr6) {
        hdr.ipv6.dstAddr = addr6;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit - 1;  // validate number of matches
    }

    table tbl_default {
        key = {
            hdr.ipv6.srcAddr : exact;
        }
        actions = { set_dst; NoAction; }
        default_action = set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_aaaa);
        size = 100;
    }

    table tbl_exact {
        key = {
            hdr.ipv6.srcAddr : exact;
        }
        actions = { set_dst; NoAction; }
        default_action = NoAction;
        size = 100;
    }

    table tbl_lpm {
        key = {
            hdr.ipv6.srcAddr : lpm;
        }
        actions = { set_dst; NoAction; }
        default_action = NoAction;
        size = 100;
    }

    table tbl_ternary {
        key = {
            hdr.ipv6.srcAddr : ternary;
        }
        actions = { set_dst; NoAction; }
        default_action = NoAction;
        size = 100;
    }

    table tbl_const_exact {
        key = {
            hdr.ipv6.srcAddr : exact;
        }
        actions = { set_dst; NoAction; }
        const entries = {
            128w0x0003_1111_2222_3333_4444_5555_6666_7777 : set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_0003);
        }
    }

    table tbl_const_lpm {
        key = {
            hdr.ipv6.srcAddr : lpm;
        }
        actions = { set_dst; NoAction; }
        const entries = {
            0x0004_1111_2222_3333_4444_5555_6666_0000 &&& 0xffff_ffff_ffff_ffff_ffff_ffff_ffff_0000 : set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_0004);
        }
    }

    table tbl_const_ternary {
        key = {
            hdr.ipv6.srcAddr : ternary;
        }
        actions = { set_dst; NoAction; }
        const entries = {
            0x0005_1111_2222_3333_4444_5555_0000_7777 &&& 0xffff_ffff_ffff_ffff_ffff_ffff_0000_ffff : set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_0005);
            0x0006_1111_2222_3333_4444_5555_6666_7777 &&& 0xffff_ffff_ffff_ffff_ffff_ffff_ffff_ffff : set_dst(128w0xffff_1111_2222_3333_4444_5555_6666_0006);
        }
    }

    ActionProfile(100) ap;
    table tbl_exact_ap {
        key = {
            hdr.ipv6.srcAddr : exact;
        }
        actions = { set_dst; NoAction; }
        psa_implementation = ap;
    }

    ActionSelector(PSA_HashAlgorithm_t.CRC32, 128, 32w16) as;
    table tbl_exact_as {
        key = {
            hdr.ipv6.srcAddr : exact;
            hdr.ipv6.srcAddr : selector;
        }
        actions = { set_dst; NoAction; }
        psa_implementation = as;
    }

    apply {
        tbl_default.apply();

        tbl_exact.apply();
        tbl_lpm.apply();
        tbl_ternary.apply();

        tbl_const_exact.apply();
        tbl_const_lpm.apply();
        tbl_const_ternary.apply();

        tbl_exact_ap.apply();
        tbl_exact_as.apply();

        send_to_port(ostd, (PortId_t) PORT0);
    }
}

control egress(inout headers hdr,
               inout empty_t meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply { }
}

control CommonDeparserImpl(packet_out packet,
                           inout headers hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv6);
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_t clone_i2e_meta,
                            out empty_t resubmit_meta,
                            out empty_t normal_meta,
                            inout headers hdr,
                            in empty_t meta,
                            in psa_ingress_output_metadata_t istd)
{
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_t clone_e2e_meta,
                           out empty_t recirculate_meta,
                           inout headers hdr,
                           in empty_t meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
