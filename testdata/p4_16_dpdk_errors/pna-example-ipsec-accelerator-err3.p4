/*
Copyright 2023 Intel Corporation

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
#include <dpdk/pna.p4>


#define inbound(meta) (istd.direction == PNA_Direction_t.NET_TO_HOST)
#define outbound(meta) (istd.direction == PNA_Direction_t.HOST_TO_NET)

//
// Packet headers.
//
header ethernet_t {
	bit<48> dst_addr;
	bit<48> src_addr;
	bit<16> ether_type;
}

#define ETHERNET_ETHERTYPE_IPV4 0x0800

header ipv4_t {
	bit<4>  version;
	bit<4>  ihl;
	bit<6>  dscp;
	bit<2>  ecn;
	bit<16> total_length;
	bit<16> identification;
	bit<3>  flags;
	bit<13> fragment_offset;
	bit<8>  ttl;
	bit<8>  protocol;
	bit<16> header_checksum;
	bit<32> src_addr;
	bit<32> dst_addr;
}

#define IP_PROTOCOL_ESP 0x32

header esp_t {
	bit<32> spi;
	bit<32> seq;
}

header platform_hdr_t {
     bit<32> my_hdr1;
}

struct headers_t {
	ethernet_t ethernet;
	ipv4_t ipv4;
	esp_t esp;
}

//
// Packet metadata.
//
struct metadata_t {
	bit<32> next_hop_id;
}

//
// Extern objects.
//
ipsec_accelerator() ipsec;

//
// Parser.
//
parser MainParserImpl(
	packet_in pkt,
	out   headers_t hdrs,
	inout metadata_t meta,
	in    pna_main_parser_input_metadata_t istd)
{
	bool from_ipsec;
	ipsec_status status;

	state start {
		from_ipsec = ipsec.from_ipsec(status);
		transition select(from_ipsec) {
			true : parse_ipv4;
			default : parse_ethernet;
		}
	}

	state parse_ethernet {
		pkt.extract(hdrs.ethernet);
		transition select(hdrs.ethernet.ether_type) {
			ETHERNET_ETHERTYPE_IPV4 : parse_ipv4;
			default : accept;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdrs.ipv4);
		transition select(hdrs.ipv4.protocol) {
			IP_PROTOCOL_ESP : parse_esp;
			default : accept;
		}
	}

	state parse_esp {
		pkt.extract(hdrs.esp);
		transition accept;
	}
}

control PreControlImpl(
    in    headers_t  hdr,
    inout metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply {
    }
}

//
// Control block.
//
control MainControlImpl(
	inout headers_t hdrs,
	inout metadata_t meta,
	in    pna_main_input_metadata_t  istd,
	inout pna_main_output_metadata_t ostd)
{

	action ipsec_enable(bit<32> sa_index) {
		ipsec.enable();
		ipsec.set_sa_index<bit<32>, bit<8>>(sa_index);
		hdrs.ethernet.setInvalid();
	}

	action ipsec_bypass() {
		ipsec.disable();
	}

	action drop() {
		drop_packet();
	}

	table inbound_table {
		key = {
			hdrs.ipv4.src_addr : exact;
			hdrs.ipv4.dst_addr : exact;
			hdrs.esp.spi       : exact;
		}

		actions = {
			ipsec_enable;
			ipsec_bypass;
			drop;
		}

		const default_action = drop;
	}

	table outbound_table {
		key = {
			hdrs.ipv4.src_addr : exact;
			hdrs.ipv4.dst_addr : exact;
		}

		actions = {
			ipsec_enable;
			ipsec_bypass;
			drop;
		}

		default_action = ipsec_bypass();
	}

	action next_hop_id_set(bit<32> next_hop_id) {
		meta.next_hop_id = next_hop_id;
	}

	table routing_table {
		key = {
			hdrs.ipv4.dst_addr : lpm;
		}

		actions = {
			next_hop_id_set;
			drop;
		}

		default_action = next_hop_id_set(0);
	}

	action next_hop_set(bit<48> dst_addr, bit<48> src_addr, bit<16> ether_type, bit<32> port_id) {
		hdrs.ethernet.setValid();
		hdrs.ethernet.dst_addr = dst_addr;
		hdrs.ethernet.src_addr = src_addr;
		hdrs.ethernet.ether_type = ether_type;
		send_to_port((PortId_t)port_id);
	}

	table next_hop_table {
		key = {
			meta.next_hop_id : exact;
		}

		actions = {
			next_hop_set;
			drop;
		}

		default_action = drop;
	}

	apply {
		if (inbound(meta)) {
			ipsec_status status;

			if (!ipsec.from_ipsec(status)) {
				// Pre-decrypt processing.
				if (hdrs.ipv4.isValid()) {
					if (hdrs.esp.isValid() ) {
						inbound_table.apply();
					}

					routing_table.apply();
					next_hop_table.apply();
				} else
					drop_packet();
			} else {
				// Post-decrypt processing.
				if (status == ipsec_status.IPSEC_SUCCESS) {
					routing_table.apply();
					next_hop_table.apply();
				} else
					drop_packet();
			}
		} else {
			ipsec_status status;

			if (!ipsec.from_ipsec(status)) {
				// Pre-encrypt processing.
				if (hdrs.ipv4.isValid()) {
					outbound_table.apply();
				} else
					drop_packet();
			} else {
				// Post-encrypt processing.
				if (status == ipsec_status.IPSEC_SUCCESS) {
					routing_table.apply();
					next_hop_table.apply();
				} else
					drop_packet();
			}
		}
	}
}

//
// Deparser.
//
control MainDeparserImpl(
	packet_out pkt,
	in    headers_t hdrs,
	in    metadata_t meta,
	in    pna_main_output_metadata_t ostd)
{
	apply {
		pkt.emit(hdrs.ethernet);
		pkt.emit(hdrs.ipv4);
		pkt.emit(hdrs.esp);
	}
}

//
// Package.
//
PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
