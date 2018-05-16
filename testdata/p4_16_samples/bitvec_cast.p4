/*
Copyright 2018-present Cavium, Inc. 

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

struct sip_t {
    bit<28>	lower28Bits;
    bit<4>	upper4Bits;
}

struct bitvec_st {
   sip_t sip;
   bit<4> version;
}

control p() {
    apply {
        bitvec_st b;
        bit<32>  ip;
        bit<36>  a;

        a = (bit<36>) b;
        b = (bitvec_st) a;
        ip = (bit<32>) b.sip;
        b.sip = (sip_t) ip;

        
    }
}