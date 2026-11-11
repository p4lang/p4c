



struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_data
	bit<32> pna_main_output_metadata_output_port
	bit<32> table_entry_index
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray per_prefix_bytes_count_0 size 0x401 initval 0x0
regarray per_prefix_pkt_bytes_count_0_packets size 0x401 initval 0x0

regarray per_prefix_pkt_bytes_count_0_bytes size 0x401 initval 0x0
regarray per_prefix_pkt_count_0 size 0x401 initval 0x0
regarray direction size 0x100 initval 0
action count_1 args none {
	jmpneq LABEL_END m.local_metadata_data 0x8
	entryid m.table_entry_index 
	regadd per_prefix_pkt_count_0 m.table_entry_index 1
	LABEL_END :	return
}

action bytecount args none {
	entryid m.table_entry_index 
	regadd per_prefix_bytes_count_0 m.table_entry_index 0x400
	return
}

action pktbytecount args none {
	entryid m.table_entry_index 
	regadd per_prefix_pkt_bytes_count_0_packets m.table_entry_index 1
	regadd per_prefix_pkt_bytes_count_0_bytes m.table_entry_index 0x400
	return
}

table ipv4_host {
	key {
		h.ipv4.dstAddr exact
	}
	actions {
		count_1
	}
	default_action count_1 args none const
	size 0x400
}


table ipv4_host_byte_count {
	key {
		h.ipv4.dstAddr exact
	}
	actions {
		bytecount
	}
	default_action bytecount args none const
	size 0x400
}


table ipv4_host_pkt_byte_count {
	key {
		h.ipv4.dstAddr exact
	}
	actions {
		pktbytecount
	}
	default_action pktbytecount args none const
	size 0x400
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	table ipv4_host
	table ipv4_host_byte_count
	table ipv4_host_pkt_byte_count
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


