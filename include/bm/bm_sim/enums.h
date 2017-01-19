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

#ifndef BM_BM_SIM_ENUMS_H_
#define BM_BM_SIM_ENUMS_H_

#include <string>
#include <unordered_map>

namespace bm {

// for this class, "name" refers to an enum entry's full name (i.e. enum name +
// "." + entry name), "enum_name" refers to an enum's name, and "entry_name"
// refers to an entry's name within an enum declaration.
class EnumMap {
 public:
  using type_t = int;

  // returns an enum entry value from its full name; will thow a
  // std::out_of_range exception if does not exist
  type_t from_name(const std::string &name) const;
  // returns an enum entry full name from the enum name and the entry value;
  // will throw a std::out_of_range exception if does not exist
  const std::string &to_name(const std::string &enum_name, type_t v) const;

  // returns true iff the enum does not already exist
  bool add_enum(const std::string &enum_name);
  // returns true iff the enum exists and the entry name / value have not been
  // taken yet
  bool add_entry(const std::string &enum_name, const std::string &entry_name,
                 type_t v);

 private:
  using EntryMap = std::unordered_map<type_t, std::string>;

  std::unordered_map<std::string, type_t> map_name_to_v{};
  std::unordered_map<std::string, EntryMap> map_enum_name_to_entries{};
};

}  // namespace bm

#endif  // BM_BM_SIM_ENUMS_H_
