
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        h3 : 16;
        h4 : 16;
    }
} 

header data_t data;

header_type counter_metadata_t {
    fields {
        counter_index_first : 16;
        counter_index_second : 16;
    }
}

metadata counter_metadata_t counter_metadata;

parser start {
    extract(data);
    return ingress;
}

action set_index(index1, index2, port) {
    modify_field(counter_metadata.counter_index_first, index1);
    modify_field(counter_metadata.counter_index_second, index2); 
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

counter count2 {
    type : packets;
    static : stats2;
    instance_count : 16384;
    min_width : 32;
}

action count_entries() {
    count(count1, counter_metadata.counter_index_first);
}

table stats {
    actions {
        count_entries;
    }
    default_action: count_entries;
}

action count_entries2() {
    count(count2, counter_metadata.counter_index_second);
}

table stats2 {
    actions {
        count_entries2;
    }
    default_action: count_entries2;
}

control ingress {
    apply(index_setter);
    apply(stats);
    apply(stats2);
}
