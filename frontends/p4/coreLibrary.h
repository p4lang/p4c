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

#ifndef _FRONTENDS_P4_CORELIBRARY_H_
#define _FRONTENDS_P4_CORELIBRARY_H_

#include "lib/cstring.h"
#include "frontends/common/model.h"

namespace P4 {

class PacketIn : public Model::Extern_Model {
 public:
    PacketIn() :
            Extern_Model("packet_in"),
            extract("extract"),
            lookahead("lookahead")
    {}
    Model::Elem extract;
    Model::Elem lookahead;
};

class PacketOut : public Model::Extern_Model {
 public:
    PacketOut() :
            Extern_Model("packet_out"),
            emit("emit")
    {}
    Model::Elem emit;
};

// Model of P4 standard library
// To be kept in sync with corelib.p4
class P4CoreLibrary : public ::Model::Model {
 protected:
    P4CoreLibrary() :
            Model("0.2"), noAction("NoAction"), exactMatch("exact"),
            ternaryMatch("ternary"), lpmMatch("lpm"), packetIn(PacketIn()),
            packetOut(PacketOut()), noError("NoError"), packetTooShort("PacketTooShort"),
            noMatch("NoMatch"), emptyStack("EmptyStack"), fullStack("FullStack"),
            overwritingHeader("OverwritingHeader") {}

 public:
    static P4CoreLibrary instance;
    ::Model::Elem noAction;

    ::Model::Elem exactMatch;
    ::Model::Elem ternaryMatch;
    ::Model::Elem lpmMatch;

    PacketIn    packetIn;
    PacketOut   packetOut;

    ::Model::Elem noError;
    ::Model::Elem packetTooShort;
    ::Model::Elem noMatch;
    ::Model::Elem emptyStack;
    ::Model::Elem fullStack;
    ::Model::Elem overwritingHeader;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CORELIBRARY_H_ */
