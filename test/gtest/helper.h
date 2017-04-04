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

#include <string>

/* preprocessing by prepending the content of core.p4 to test program */
std::string with_core_p4 (std::string pgm) {
    const std::string core_p4 = " \
error { \
    NoError, \
    PacketTooShort, \
    NoMatch, \
    StackOutOfBounds, \
    OverwritingHeader, \
    HeaderTooShort, \
    ParserTimeout \
} \
extern packet_in { \
    void extract<T>(out T hdr); \
    void extract<T>(out T variableSizeHeader, \
                    in bit<32> variableFieldSizeInBits); \
    T lookahead<T>(); \
    void advance(in bit<32> sizeInBits); \
    bit<32> length(); \
} \
extern packet_out { \
    void emit<T>(in T hdr); \
    void emit<T>(in bool condition, in T data); \
} \
@name(\"NoAction\") \
action NoAction() {} \
match_kind { \
    exact, \
    ternary, \
    lpm \
} \n";
    return core_p4 + pgm;
}
