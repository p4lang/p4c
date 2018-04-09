/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
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

action _drop() { drop(); }

table set_default_behavior_drop {
    actions {
        _drop;
    }
    default_action: _drop;
}

action noop() { }
action setb1(val, port) {
    modify_field(data.b1, val);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setb1;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    // Unless some later action sets standard_metadata.egress_spec to
    // the value corresponding to an output port, the packet will be
    // dropped at the end of ingress.
    apply(set_default_behavior_drop);
    if (data.b2 == data.b3 and data.b4 == 10) {
        apply(test1);
    } else {
        apply(test2);
    }
}
