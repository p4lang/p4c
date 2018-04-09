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

header_type foo_t {
    fields {
        src : 8;
        dst : 8;
    }
}

parser start {
    return ingress;
}

// This test passes local_port (an untyped variable) to recirculate
// which expects a field list. It exposed issue #747, where the code
// expected that calling ::error will exit out of the function.
action local_recirc(local_port) {
    resubmit( local_port );
}
table do_local_recirc {
    actions { local_recirc; }
}

control ingress {
  apply(do_local_recirc);
}


control egress { }
