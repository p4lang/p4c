

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

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<64> Ingress_tmp
	bit<64> Ingress_tmp_0
	bit<64> Ingress_tmp_1
	bit<64> Ingress_tmp_2
	bit<16> Ingress_tmp_4
	bit<64> Ingress_tmp_6
	bit<64> Ingress_tmp_7
	bit<64> Ingress_tmp_8
	bit<64> Ingress_tmp_9
	bit<64> Ingress_tmp_10
	bit<48> Ingress_tmp_13
	bit<64> Ingress_tmp_15
	bit<64> Ingress_tmp_16
	bit<64> Ingress_s
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray port_pkt_ip_bytes_in_0 size 0x200 initval 0
apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	INGRESSPARSERIMPL_ACCEPT :	mov m.psa_ingress_output_metadata_egress_port 0x0
	validate h.ipv4
	mov h.ipv4.totalLen 0xE
	jmpnv LABEL_END h.ipv4
	regrd m.Ingress_tmp_16 port_pkt_ip_bytes_in_0 m.psa_ingress_input_metadata_ingress_port
	mov m.Ingress_s m.Ingress_tmp_16
	mov m.Ingress_tmp m.Ingress_s
	and m.Ingress_tmp 0xFFFFFFFFFFFF
	mov m.Ingress_tmp_0 m.Ingress_tmp_16
	shr m.Ingress_tmp_0 0x30
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	and m.Ingress_tmp_1 0xFFFF
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	and m.Ingress_tmp_2 0xFFFF
	mov m.Ingress_tmp_4 m.Ingress_tmp_2
	add m.Ingress_tmp_4 0x1
	mov m.Ingress_tmp_6 m.Ingress_tmp_4
	shl m.Ingress_tmp_6 0x30
	mov m.Ingress_tmp_7 m.Ingress_tmp_6
	and m.Ingress_tmp_7 0xFFFF000000000000
	mov m.Ingress_s m.Ingress_tmp
	or m.Ingress_s m.Ingress_tmp_7
	mov m.Ingress_tmp_8 m.Ingress_s
	and m.Ingress_tmp_8 0xFFFF000000000000
	mov m.Ingress_tmp_9 m.Ingress_s
	and m.Ingress_tmp_9 0xFFFFFFFFFFFF
	mov m.Ingress_tmp_10 m.Ingress_tmp_9
	and m.Ingress_tmp_10 0xFFFFFFFFFFFF
	mov m.Ingress_tmp_13 m.Ingress_tmp_10
	add m.Ingress_tmp_13 h.ipv4.totalLen
	mov m.Ingress_tmp_15 m.Ingress_tmp_13
	and m.Ingress_tmp_15 0xFFFFFFFFFFFF
	mov m.Ingress_s m.Ingress_tmp_8
	or m.Ingress_s m.Ingress_tmp_15
	mov m.Ingress_tmp_16 m.Ingress_s
	regwr port_pkt_ip_bytes_in_0 m.psa_ingress_input_metadata_ingress_port m.Ingress_tmp_16
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


