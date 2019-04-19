/* Copyright 2017-present Kentaro Ebisawa <ebisawa@jp.toyota-itc.com>
 * This is a sample code to reproduce below p4c error using variable lengh fields.
 * Compiler Bug: 48: Unexpected type for constant varbit<0>
 */

/* Written in P4_14 */

// CAN data
header_type can_data_t {
    fields {
        id : 16;
        len : 16;  // length of value
        value : *;
    }
    length : 4+len;
    max_length : 12;
}
header can_data_t can_data;

//// Parser definition
parser start {
    return parse_p4can_data;
}

parser parse_p4can_data {
    extract(can_data);
    return ingress;
}

action value_add() {
	//modify_field(can_data.value, can_data.value); // no-error
	//modify_field(can_data.value, can_data.value - 1); // error
	//modify_field(can_data.value, can_data.value + 1); // error
	modify_field(can_data.value, can_data.value * 2); // error
}

table t_can {
    reads {
        can_data.id: exact;
    }
    actions {value_add;}
}

control ingress {
    apply(t_can);
}
