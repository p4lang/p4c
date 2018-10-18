header_type user_metadata_t {
    fields {
	pkt_count: 32;
 	status_cycle: 2; // should only be 0,1,2,3
	global_count : 64;
    }
}
metadata user_metadata_t md;

parser start {
    return ingress;
}

action maintain_2bit_variable(){
	shift_right(md.status_cycle, md.pkt_count, 4);
	shift_left(md.status_cycle, md.pkt_count, 2);
}
action incr_global_count() {
	shift_right(md.global_count, md.pkt_count, 4);
	shift_left(md.global_count, md.pkt_count, 2);
}
table tb_maintain_2bit_variable{
	actions {maintain_2bit_variable; incr_global_count;}
	default_action: maintain_2bit_variable;
}

control ingress{
	apply(tb_maintain_2bit_variable);
}
control egress {
}
