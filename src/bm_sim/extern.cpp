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

#include <bm/bm_sim/extern.h>

#include <string>

namespace bm {

ExternFactoryMap *
ExternFactoryMap::get_instance() {
  static ExternFactoryMap instance;
  return &instance;
}

int
ExternFactoryMap::register_extern_type(const char *extern_type_name,
                                       ExternFactoryFn fn) {
  const std::string str_name = std::string(extern_type_name);
  auto it = factory_map.find(str_name);
  if (it != factory_map.end()) return 0;
  factory_map[str_name] = std::move(fn);
  return 1;
}

std::unique_ptr<ExternType>
ExternFactoryMap::get_extern_instance(
    const std::string &extern_type_name) const {
  auto it = factory_map.find(extern_type_name);
  if (it == factory_map.end()) return nullptr;
  return it->second();
}

void
ExternType::_set_name_and_id(const std::string &name, p4object_id_t id) {
  this->name = name;
  this->id = id;
}

}  // namespace bm
