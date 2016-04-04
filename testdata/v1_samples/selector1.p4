header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        b1 : 8;
        b2 : 8;
        b3 : 8;
        b4 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

field_list sel_fields {
    data.f1;
    data.f2;
    data.f3;
    data.f4;
}

field_list_calculation sel_hash {
    input {
        sel_fields;
    }
    algorithm : crc16;
    output_width : 14;
}

action_selector sel {
    selection_key : sel_hash;
    selection_mode : fair;
}

action noop() { }
action setf1(val) { modify_field(data.f1, val); }

action_profile sel_profile {
    actions {
        noop;
        setf1;
    }
    size : 16384;
    dynamic_action_selection : sel;
}

table test1 {
    reads {
        data.b1 : exact;
    }
    action_profile : sel_profile;
    size : 1024;
}

control ingress {
    apply(test1);
}
