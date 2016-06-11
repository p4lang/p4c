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

extern Checksum16 
{
    // prepare unit
    void reset();
    // append data to be checksummed
    void append<D>(in D d); // same as { append(true, d); }
    // conditionally append data to be checksummed
    void append<D>(in bool condition, in D d);
    // get the checksum of all data appended since the last reset
    bit<16> get();
}
