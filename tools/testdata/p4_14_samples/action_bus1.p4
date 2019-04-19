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

/* try to fill the action bus with 32-bit values... */
#define REP5(M, ...) M(__VA_ARGS__, 1) M(__VA_ARGS__, 2) M(__VA_ARGS__, 3) \
                     M(__VA_ARGS__, 4) M(__VA_ARGS__, 5)
#define REP8(M, ...) M(__VA_ARGS__, 1) M(__VA_ARGS__, 2) M(__VA_ARGS__, 3) \
                     M(__VA_ARGS__, 4) M(__VA_ARGS__, 5) M(__VA_ARGS__, 6) \
                     M(__VA_ARGS__, 7) M(__VA_ARGS__, 8)

header_type data_t {
    fields {
#define F32(A,B) f##A##_##B : 32;
REP8(REP5,F32)
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
#define MF(A,B) modify_field(data.f##A##_##B, v##B);
#define SET(_,A) action set##A(v1,v2,v3,v4,v5) { REP5(MF,A) }
REP8(SET)

#define TBL(_,A) table tbl##A { \
    reads {                     \
        data.f##A##_1 : exact;  \
    }                           \
    actions {                   \
        set##A;                 \
        noop;                   \
    }                           \
}
REP8(TBL)
#define USE_TBL(_,A)     apply(tbl##A);

control ingress {
    REP8(USE_TBL)
}
