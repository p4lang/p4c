/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/enums.h>

#include <string>

namespace bm {

bool
EnumMap::add_enum(const std::string &enum_name) {
  auto it = map_enum_name_to_entries.find(enum_name);
  if (it != map_enum_name_to_entries.end()) return false;
  map_enum_name_to_entries[enum_name] = {};
  return true;
}

bool
EnumMap::add_entry(const std::string &enum_name, const std::string &entry_name,
                   type_t v) {
  // make sure that the enum exists
  auto it = map_enum_name_to_entries.find(enum_name);
  if (it == map_enum_name_to_entries.end()) return false;
  auto &entry_map = it->second;
  // make sure that the value is not used yet for this enum
  auto it2 = entry_map.find(v);
  if (it2 != entry_map.end()) return false;
  auto name = enum_name + "." + entry_name;
  // make sure that the entry name is not used yet for this enum
  auto it3 = map_name_to_v.find(name);
  if (it3 != map_name_to_v.end()) return false;
  // add necessary entries to maps
  entry_map[v] = name;
  map_name_to_v[name] = v;
  return true;
}

EnumMap::type_t
EnumMap::from_name(const std::string &name) const {
  return map_name_to_v.at(name);
}

const std::string &
EnumMap::to_name(const std::string &enum_name, type_t v) const {
  const auto &entry_map = map_enum_name_to_entries.at(enum_name);
  return entry_map.at(v);
}

}  // namespace bm
