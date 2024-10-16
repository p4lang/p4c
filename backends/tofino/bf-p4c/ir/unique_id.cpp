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

#include "unique_id.h"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"

namespace P4 {

static const char *attached_id_to_str[] = {
    "", "tind", "idletime", "stats", "meter", "selector", "salu", "action_data"
};

static const char *speciality_to_str[] = {
    "", "atcam", "dleft"
};

void UniqueAttachedId::toJSON(P4::JSONGenerator &json) const {
    json << json.indent << "\"name\": " << name << ",\n"
         << json.indent << "\"type\": " << type << ",\n";
}

UniqueAttachedId UniqueAttachedId::fromJSON(P4::JSONLoader &json) {
    UniqueAttachedId uai;
    if (!json.json) return uai;
    json.load("name", uai.name);
    json.load("type", uai.type);
    return uai;
}

std::string UniqueAttachedId::build_name() const {
    std::string rv = "";
    if (type == INVALID)
        return rv;
    rv += attached_id_to_str[type];
    if (name_valid())
        rv += "." + name;
    return rv;
}

bool UniqueId::operator==(const UniqueId &ui) const {
    return name == ui.name &&
           stage_table == ui.stage_table &&
           logical_table == ui.logical_table &&
           is_gw == ui.is_gw &&
           speciality == ui.speciality &&
           a_id == ui.a_id;
}

bool UniqueId::operator<(const UniqueId &ui) const {
    if (name != ui.name)
        return name < ui.name;
    if (stage_table != ui.stage_table)
        return stage_table < ui.stage_table;
    if (logical_table != ui.logical_table)
        return logical_table < ui.logical_table;
    if (is_gw != ui.is_gw)
        return is_gw < ui.is_gw;
    if (speciality != ui.speciality)
        return speciality < ui.speciality;
    if (a_id != ui.a_id)
        return a_id < ui.a_id;
    return false;
}

bool UniqueId::equal_table(const UniqueId &ui) const {
    return name == ui.name &&
           stage_table == ui.stage_table &&
           logical_table == ui.logical_table &&
           speciality == ui.speciality;
}

UniqueId UniqueId::base_match_id() const {
    UniqueId rv;
    rv.name = name;
    rv.stage_table = stage_table;
    rv.logical_table = logical_table;
    rv.speciality = speciality;
    return rv;
}

std::string UniqueId::build_name() const {
    std::string rv = "";
    rv += name + "";
    if (speciality != NONE) {
        rv += "$";
        rv += speciality_to_str[static_cast<int>(speciality)];
    }
    if (logical_table_used())
       rv += "$lt" + std::to_string(logical_table);
    if (stage_table_used())
        rv += "$st" + std::to_string(stage_table);
    if (is_gw)
        rv += "$gw";
    if (a_id)
        rv += "$" + a_id.build_name();
    return rv;
}

std::ostream &operator <<(std::ostream &out, const UniqueId &ui) {
    out << ui.build_name();
    return out;
}

}  // namespace P4
