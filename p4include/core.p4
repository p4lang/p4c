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

/* This is the P4-16 core library, which declares some built-in P4 constructs using P4 */

#ifndef _CORE_P4_
#define _CORE_P4_

error {
    NoError,           // no error
    PacketTooShort,    // not enough bits in packet for extract
    NoMatch,           // match expression has no matches
    StackOutOfBounds,  // reference to invalid element of a header stack
    OverwritingHeader, // one header is extracted twice
    HeaderTooShort,    // extracting too many bits in a varbit field
    ParserTimeout      // parser execution time limit exceeded
}

extern packet_in {
    // T must be a fixed-size header type
    void extract<T>(out T hdr);
    // T must be a header containing exactly 1 varbit field
    void extract<T>(out T variableSizeHeader,
                    in bit<32> variableFieldSizeInBits);
    // does not advance the cursor
    T lookahead<T>();
    // skip this many bits from packet
    void advance(in bit<32> sizeInBits);
    // packet length in bytes
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
    void emit<T>(in bool condition, in T data);
}

// TODO: remove from this file, convert to built-in
extern void verify(in bool check, in error toSignal);

@name("NoAction")
action NoAction() {}

match_kind {
    exact,
    ternary,
    lpm
}

#endif  /* _CORE_P4_ */
