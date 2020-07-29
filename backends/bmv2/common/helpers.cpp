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

#include "helpers.h"

namespace BMV2 {

/// constant definition for bmv2
const cstring MatchImplementation::selectorMatchTypeName = "selector";
const cstring MatchImplementation::rangeMatchTypeName = "range";
const cstring MatchImplementation::optionalMatchTypeName = "optional";
const unsigned TableAttributes::defaultTableSize = 1024;
const cstring V1ModelProperties::jsonMetadataParameterName = "standard_metadata";
const cstring V1ModelProperties::validField = "$valid$";

Util::IJson* nodeName(const CFG::Node* node) {
    if (node->name.isNullOrEmpty())
        return Util::JsonValue::null;
    else
        return new Util::JsonValue(node->name);
}

Util::JsonArray* mkArrayField(Util::JsonObject* parent, cstring name) {
    auto result = new Util::JsonArray();
    parent->emplace(name, result);
    return result;
}

Util::JsonArray* mkParameters(Util::JsonObject* object) {
    return mkArrayField(object, "parameters");
}

Util::JsonObject* mkPrimitive(cstring name, Util::JsonArray* appendTo) {
    auto result = new Util::JsonObject();
    result->emplace("op", name);
    appendTo->append(result);
    return result;
}

Util::JsonObject* mkPrimitive(cstring name) {
    auto result = new Util::JsonObject();
    result->emplace("op", name);
    return result;
}

cstring stringRepr(big_int value, unsigned bytes) {
    cstring sign = "";
    std::stringstream r;
    cstring filler = "";
    if (value < 0) {
        value =- value;
        sign = "-"; }
    r << std::hex << value;

    if (bytes > 0) {
        int digits = bytes * 2 - r.str().size();
        BUG_CHECK(digits >= 0, "Cannot represent %1% on %2% bytes", value, bytes);
        filler = std::string(digits, '0');
    }
    return sign + "0x" + filler + r.str();
}

unsigned nextId(cstring group) {
    static std::map<cstring, unsigned> counters;
    return counters[group]++;
}

}  // namespace BMV2


