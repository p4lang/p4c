// Header declarations

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

header ethernet_t ethernet;
metadata meta_t meta;

// Parser

parser start {
    return ethernet;
}

parser ethernet {
    extract(ethernet);
    return ingress;
}

counter ethernet {
    type : packets;
    direct : ethernet;
}

action ethernet() {
    modify_field(standard_metadata.egress_spec, standard_metadata.ingress_port);
}

table ethernet {
    actions {
		ethernet;
    }
}

control ingress {
	apply(ethernet);
}
