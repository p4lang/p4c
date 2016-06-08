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

parser start {
    return ingress;
}

header_type meta_t {
  fields {
    f : 32;
  }
}

header meta_t meta;

counter my_indirect_counter {
    type: packets;
    static: m_table;
    instance_count: 16;
}

meter my_indirect_meter {
    type: packets;
    static: m_table;
    instance_count: 16;
}

register my_register {
    width: 32;
    static: m_table;
    instance_count: 16;
}

action m_action() {
    count(my_indirect_counter, 1);
    execute_meter(my_indirect_meter, 1, meta.f);
    register_write(my_register, 1, 0xab);
}

table m_table {
    actions { m_action; }
    size : 1024;
}

control ingress {
    apply(m_table);
}

control egress {
}
