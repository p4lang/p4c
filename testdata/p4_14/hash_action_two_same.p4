
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        h3 : 16;
        h4 : 16;
        h5 : 16;
        h6 : 16;
        color_1 : 8;
    }
} 

header data_t data;

header_type counter_metadata_t {
    fields {
        counter_index : 16;
    }
}

header_type meter_metadata_t {
    fields {
        meter_index : 16;
    }
}

metadata counter_metadata_t counter_metadata;
metadata meter_metadata_t meter_metadata;

parser start {
    extract(data);
    return ingress;
}

action set_index(index, port) {
    modify_field(counter_metadata.counter_index, index);
    modify_field(meter_metadata.meter_index, index);
    modify_field(standard_metadata.egress_spec, port);
}

table index_setter {
    reads {
        data.f1 : exact;
        data.f2 : exact;
    }
    actions {
        set_index;    
    }
    size : 2048;
}

counter count1 {
    type : packets;
    static : stats;
    instance_count : 16384;
    min_width : 32;
}

meter meter1 {
    type : bytes;
    static : stats;
    //result : data.color_1;
    instance_count : 1024;
}

action count_entries() {
    count(count1, counter_metadata.counter_index);
    execute_meter(meter1, meter_metadata.meter_index, data.color_1);
}

table stats {
    actions {
        count_entries;
    }
}

control ingress {
    apply(index_setter);
    apply(stats);
}
