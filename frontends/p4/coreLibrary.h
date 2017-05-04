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
#include "ir/ir.h"

namespace P4 {
enum class StandardExceptions {
    NoError,
    PacketTooShort,
    NoMatch,
    StackOutOfBounds,
    OverwritingHeader,
    HeaderTooShort,
    ParserTimeout,
};
}  // namespace P4

inline std::ostream& operator<<(std::ostream &out, P4::StandardExceptions e) {
    switch (e) {
        case P4::StandardExceptions::NoError:
            out << "NoError";
            break;
        case P4::StandardExceptions::PacketTooShort:
            out << "PacketTooShort";
            break;
        case P4::StandardExceptions::NoMatch:
            out << "NoMatch";
            break;
        case P4::StandardExceptions::StackOutOfBounds:
            out << "StackOutOfBounds";
            break;
        case P4::StandardExceptions::OverwritingHeader:
            out << "OverwritingHeader";
            break;
        case P4::StandardExceptions::HeaderTooShort:
            out << "HeaderTooShort";
            break;
        case P4::StandardExceptions::ParserTimeout:
            out << "ParserTimeout";
            break;
        default:
            BUG("Unhandled case");
    }
    return out;
}

namespace P4 {

class PacketIn : public Model::Extern_Model {
 public:
    PacketIn() :
            Extern_Model("packet_in"),
            extract("extract"), lookahead("lookahead"),
            advance("advance"), length("length") {}
    Model::Elem extract;
    Model::Elem lookahead;
    Model::Elem advance;
    Model::Elem length;
    int extractSecondArgSize = 32;
};

class PacketOut : public Model::Extern_Model {
 public:
    PacketOut() : Extern_Model("packet_out"), emit("emit") {}
    Model::Elem emit;
};

class P4Exception_Model : public ::Model::Elem {
 public:
    const StandardExceptions exc;
    explicit P4Exception_Model(StandardExceptions exc) : ::Model::Elem(""), exc(exc) {
        std::stringstream str;
        str << exc;
        name = str.str();
    }
};

// Model of P4 core library
// To be kept in sync with core.p4
class P4CoreLibrary : public ::Model::Model {
 protected:
    P4CoreLibrary() :
            Model("0.2"), noAction("NoAction"), exactMatch("exact"),
            ternaryMatch("ternary"), lpmMatch("lpm"), packetIn(PacketIn()),
            packetOut(PacketOut()), noError(StandardExceptions::NoError),
            packetTooShort(StandardExceptions::PacketTooShort),
            noMatch(StandardExceptions::NoMatch),
            stackOutOfBounds(StandardExceptions::StackOutOfBounds),
            overwritingHeader(StandardExceptions::OverwritingHeader),
            headerTooShort(StandardExceptions::HeaderTooShort) {}

 public:
    static P4CoreLibrary instance;
    ::Model::Elem noAction;

    ::Model::Elem exactMatch;
    ::Model::Elem ternaryMatch;
    ::Model::Elem lpmMatch;

    PacketIn    packetIn;
    PacketOut   packetOut;

    P4Exception_Model noError;
    P4Exception_Model packetTooShort;
    P4Exception_Model noMatch;
    P4Exception_Model stackOutOfBounds;
    P4Exception_Model overwritingHeader;
    P4Exception_Model headerTooShort;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CORELIBRARY_H_ */
