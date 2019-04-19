// Header declarations

field_list test1_digest {
    ethernet.dstAddr;
    standard_metadata;
}

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type meta_t {
    fields {
        val16 : 16;
    }
}

// declare the special timestamp metadata
header_type intrinsic_metadata_t {
    fields {
        ingress_global_timestamp : 48;
    }
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;
metadata meta_t meta;

// Parser

parser start {
    set_metadata(standard_metadata.egress_spec, standard_metadata.ingress_port);
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}


action test_action() {
    generate_digest(0x666, test1_digest);
}

table tbl0 {
    actions {
		test_action;
    }
}

control ingress {
	apply(tbl0);
}
