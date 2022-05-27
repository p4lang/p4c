

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<32> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct psa_ingress_output_metadata_t {
	bit<8> class_of_service
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_output_metadata_t {
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
}

struct psa_egress_deparser_input_metadata_t {
	bit<32> egress_port
}

struct user_meta_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> local_metadata_data
	bit<48> Ingress_tmp
	bit<16> Ingress_tmp_0
	bit<48> user_meta_t_Ingress_tmp
	bit<16> user_meta_t_Ingress_tmp_0
	bit<8> ipv4_t_version_ihl
	bit<8> ipv4_t_diffserv
	bit<32> ipv4_t_totalLen
	bit<16> ipv4_t_identification
	bit<16> ipv4_t_flags_fragOffset
	bit<8> ipv4_t_ttl
	bit<8> ipv4_t_protocol
	bit<16> ipv4_t_hdrChecksum
	bit<32> ipv4_t_srcAddr
	bit<32> ipv4_t_dstAddr
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header Ingress_tmp_1 instanceof ipv4_t

action NoAction args none {
	return
}

action a1 args none {
	mov m.Ingress_tmp h.ethernet.srcAddr
	mov m.Ingress_tmp_0 h.ethernet.etherType
	mov m.user_meta_t_Ingress_tmp m.Ingress_tmp
	mov m.user_meta_t_Ingress_tmp_0 m.Ingress_tmp_0
	mov m.ipv4_t_version_ihl h.Ingress_tmp_1.version_ihl
	mov m.ipv4_t_diffserv h.Ingress_tmp_1.diffserv
	mov m.ipv4_t_totalLen h.Ingress_tmp_1.totalLen
	mov m.ipv4_t_identification h.Ingress_tmp_1.identification
	mov m.ipv4_t_flags_fragOffset h.Ingress_tmp_1.flags_fragOffset
	mov m.ipv4_t_ttl h.Ingress_tmp_1.ttl
	mov m.ipv4_t_protocol h.Ingress_tmp_1.protocol
	mov m.ipv4_t_hdrChecksum h.Ingress_tmp_1.hdrChecksum
	mov m.ipv4_t_srcAddr h.Ingress_tmp_1.srcAddr
	mov m.ipv4_t_dstAddr h.Ingress_tmp_1.dstAddr
	hash crc32 m.local_metadata_data  m.user_meta_t_Ingress_tmp m.ipv4_t_dstAddr
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


