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

#include "clot.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "lib/exceptions.h"
#include "lib/cstring.h"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "bf-p4c/phv/phv_fields.h"

std::map<gress_t, int> Clot::tag_count;

cstring Clot::toString() const {
    std::stringstream tmp;
    tmp << "clot " << gress << "::" << tag;
    return tmp.str();
}

Clot::Clot(cstring name) {
    std::string str(name);

    // Ensure we have the "clot " prefix.
    if (str.length() <= 5 || str.substr(0, 5) != "clot ")
        BUG("Invalid CLOT: '%s'", name);

    // Ensure we have the "::" separator, and that it is immediately followed with a digit.
    auto identifier = str.substr(5);
    auto sep_pos = identifier.find("::");

    BUG_CHECK(sep_pos != std::string::npos &&
              identifier.length() > sep_pos + 2 &&
              isdigit(identifier[sep_pos + 2]),
        "Invalid CLOT: '%s'", name);

    // Parse out the gress.
    gress_t gress;
    BUG_CHECK(identifier.substr(0, sep_pos) >> gress, "Invalid CLOT: '%s'", name);

    // Parse out the CLOT id.
    char* end = nullptr;
    auto v = std::strtol(identifier.substr(sep_pos + 2).c_str(), &end, 10);
    if (*end) BUG("Invalid CLOT: '%s'", name);

    this->gress = gress;
    this->tag = v;
}

void Clot::add_slice(cstring parser_state, FieldKind kind, const PHV::FieldSlice* slice) {
    const PHV::Field* field = slice->field();

    switch (kind) {
    case MODIFIED:
        phv_fields_.insert(field);
        break;

    case READONLY:
        phv_fields_.insert(field);
        break;

    case CHECKSUM:
        BUG_CHECK(slice->is_whole_field(),
                  "Attempted to allocate checksum slice to CLOT %d: %s",
                  tag,
                  slice->shortString());
        checksum_fields_.insert(field);
        break;

    case UNUSED:
        break;
    }

    parser_state_to_slices_[parser_state].push_back(slice);
    fields_to_slices_[field] = slice;
    LOG5("Adding slice " << slice->shortString() << " to parser state " << parser_state);
}

unsigned Clot::length_in_bytes(cstring parser_state) const {
    unsigned length_in_bits = 0;
    BUG_CHECK(parser_state_to_slices_.count(parser_state),
              "CLOT %d is not present in parser state %s",
              tag, parser_state);
    for (const auto* slice : parser_state_to_slices_.at(parser_state))
        length_in_bits += slice->range().size();

    BUG_CHECK(length_in_bits % 8 == 0,
              "CLOT %d has %d bits, which is not a whole number of bytes",
              tag, length_in_bits);

    return length_in_bits / 8;
}

unsigned Clot::bit_offset(cstring parser_state,
                          const PHV::FieldSlice* slice) const {
    unsigned offset = 0;
    BUG_CHECK(parser_state_to_slices_.count(parser_state),
              "CLOT %d is not present in parser state %s",
              tag, parser_state);
    for (const auto* mem : parser_state_to_slices_.at(parser_state)) {
        if (PHV::FieldSlice::equal(slice, mem)) return offset;
        offset += mem->range().size();
    }

    BUG("Field %s not in %s when issued by parser state %s",
        slice->shortString(), toString(), parser_state);
}

unsigned Clot::byte_offset(cstring parser_state,
                           const PHV::FieldSlice* slice) const {
    return bit_offset(parser_state, slice) / 8;
}

bool Clot::is_phv_field(const PHV::Field* field) const {
    return phv_fields_.count(field);
}

bool Clot::is_checksum_field(const PHV::Field* field) const {
    return checksum_fields_.count(field);
}

bool Clot::has_slice(const PHV::FieldSlice* slice) const {
    auto field = slice->field();
    if (!fields_to_slices_.count(field)) return false;
    return PHV::FieldSlice::equal(fields_to_slices_.at(field), slice);
}

bool Clot::is_first_field_in_clot(const PHV::Field* field) const {
    for (const auto& kv : parser_state_to_slices_) {
        const PHV::Field* first_field = kv.second.at(0)->field();
        if (first_field == field)
            return true;
    }
    return false;
}

void Clot::set_slices(cstring parser_state, const std::vector<const PHV::FieldSlice*>& slices) {
    parser_state_to_slices_[parser_state] = slices;

    // Check that all fields in the slices we were given is a subset of the existing fields. At the
    // same time, start fixing up fields_to_slices_.
    std::set<const PHV::Field*> fields;
    for (auto slice : slices) {
        auto field = slice->field();
        BUG_CHECK(fields_to_slices_.count(field),
                  "Found a foreign field %s when setting slices for CLOT %d",
                  field->name, tag);

        fields.insert(field);
        fields_to_slices_[field] = slice;
    }

    std::set<const PHV::Field*> fields_from_other_parser_states;
    for (const auto& kv : parser_state_to_slices_) {
        if (kv.first == parser_state) continue;

        auto slices = kv.second;
        for (const auto& slice : slices) {
            auto field = slice->field();
            fields_from_other_parser_states.insert(field);
        }
    }

    // Finish fixing up fields_to_slices_.
    for (auto it = fields_to_slices_.begin(); it != fields_to_slices_.end(); ) {
        if (fields.count(it->first) || fields_from_other_parser_states.count(it->first))
            ++it;
        else
            it = fields_to_slices_.erase(it);
    }

    // Fix up phv_fields_.
    for (auto it = phv_fields_.begin(); it != phv_fields_.end(); ) {
        if (fields.count(*it) || fields_from_other_parser_states.count(*it))
            ++it;
        else
            it = phv_fields_.erase(it);
    }

    // Fix up checksum_fields_.
    for (auto it = checksum_fields_.begin(); it != checksum_fields_.end(); ) {
        if (fields.count(*it) || fields_from_other_parser_states.count(*it))
            ++it;
        else
            it = checksum_fields_.erase(it);
    }
}

void Clot::toJSON(JSONGenerator& json) const {
    json << *this;
}

/* static */ Clot* Clot::fromJSON(JSONLoader& json) {
    if (auto* v = json.json->to<JsonString>())
        return new Clot(cstring(v->c_str()));
    BUG("Couldn't decode JSON value to clot");
    return new Clot();
}

std::ostream& operator<<(std::ostream& out, const Clot& clot) {
    return out << "CLOT " << clot.tag;
}

std::ostream& operator<<(std::ostream& out, const Clot* clot) {
    if (clot)
        return out << *clot;
    else
        return out << "(nullptr)";
}

P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const Clot& clot) {
    return out << clot.toString();
}

P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const Clot* clot) {
    if (clot)
        return out << *clot;
    else
        return out << "(nullptr)";
}
