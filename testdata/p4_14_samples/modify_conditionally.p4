header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

header_type metadata_global_t{
    fields{
        do_goto_table: 1;
        goto_table_id: 8;
    }
}

metadata metadata_global_t metadata_global;

action table0_actionlist(
    do_goto_table, goto_table_id
)
{
    modify_field( metadata_global.do_goto_table, do_goto_table );
    modify_field_conditionally( metadata_global.goto_table_id, do_goto_table, goto_table_id );
}

table table0 {
    reads {
        ethernet.etherType:ternary;
    }
    actions {
        table0_actionlist;
    }
    size: 2000;
}

control ingress{
    if(metadata_global.goto_table_id == 0) {
        apply( table0 );
    }
}

control egress{
}
