/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "match_register.h"

#include <sstream>

#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"

int MatchRegister::s_id = 0;

cstring MatchRegister::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

MatchRegister::MatchRegister() : name(""), size(0), id(0) {}

MatchRegister::MatchRegister(cstring n) : name(n), id(s_id++) {
    if (name.find("byte"))
        size = 1;
    else if (name.find("half"))
        size = 2;
    else
        BUG("Invalid parser match register %s", name);
}

void MatchRegister::toJSON(JSONGenerator &json) const { json << *this; }

/* static */
MatchRegister MatchRegister::fromJSON(JSONLoader &json) {
    if (auto *v = json.json->to<JsonString>()) return MatchRegister(cstring(v->c_str()));
    BUG("Couldn't decode JSON value to parser match register");
    return MatchRegister();
}

P4::JSONGenerator &operator<<(P4::JSONGenerator &out, const MatchRegister &c) {
    return out << c.toString();
}
