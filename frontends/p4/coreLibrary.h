/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CORELIBRARY_H_
#define FRONTENDS_P4_CORELIBRARY_H_

#include "frontends/common/model.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4 {
enum class StandardExceptions {
    NoError,
    PacketTooShort,
    NoMatch,
    StackOutOfBounds,
    HeaderTooShort,
    ParserTimeout,
};
}  // namespace P4

namespace P4 {

inline std::ostream &operator<<(std::ostream &out, P4::StandardExceptions e) {
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

}  // namespace P4

namespace P4 {

using namespace literals;

class PacketIn : public Model::Extern_Model {
 public:
    PacketIn()
        : Extern_Model("packet_in"_cs),
          extract("extract"_cs),
          lookahead("lookahead"_cs),
          advance("advance"_cs),
          length("length"_cs) {}
    Model::Elem extract;
    Model::Elem lookahead;
    Model::Elem advance;
    Model::Elem length;
    int extractSecondArgSize = 32;
};

class PacketOut : public Model::Extern_Model {
 public:
    PacketOut() : Extern_Model("packet_out"_cs), emit("emit"_cs) {}
    Model::Elem emit;
};

class P4Exception_Model : public ::P4::Model::Elem {
 public:
    const StandardExceptions exc;
    explicit P4Exception_Model(StandardExceptions exc)
        : ::P4::Model::Elem(cstring::empty), exc(exc) {
        std::stringstream str;
        str << exc;
        name = str.str();
    }
};

// Model of P4 core library
// To be kept in sync with core.p4
class P4CoreLibrary : public ::P4::Model::Model {
 protected:
    // NOLINTBEGIN(bugprone-throw-keyword-missing)
    P4CoreLibrary()
        : noAction("NoAction"_cs),
          exactMatch("exact"_cs),
          ternaryMatch("ternary"_cs),
          lpmMatch("lpm"_cs),
          noError(StandardExceptions::NoError),
          packetTooShort(StandardExceptions::PacketTooShort),
          noMatch(StandardExceptions::NoMatch),
          stackOutOfBounds(StandardExceptions::StackOutOfBounds),
          headerTooShort(StandardExceptions::HeaderTooShort) {}
    // NOLINTEND(bugprone-throw-keyword-missing)

 public:
    static P4CoreLibrary &instance() {
        static P4CoreLibrary *corelib = new P4CoreLibrary();
        return *corelib;
    }
    ::P4::Model::Elem noAction;

    ::P4::Model::Elem exactMatch;
    ::P4::Model::Elem ternaryMatch;
    ::P4::Model::Elem lpmMatch;

    PacketIn packetIn;
    PacketOut packetOut;

    P4Exception_Model noError;
    P4Exception_Model packetTooShort;
    P4Exception_Model noMatch;
    P4Exception_Model stackOutOfBounds;
    P4Exception_Model headerTooShort;
};

}  // namespace P4

#endif /* FRONTENDS_P4_CORELIBRARY_H_ */
