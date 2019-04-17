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

#include <bm/bm_sim/parser_error.h>

#include <algorithm>  // for std::max_element
#include <cassert>
#include <string>

namespace bm {

constexpr ErrorCode::type_t ErrorCode::INVALID_V;

bool
ErrorCodeMap::add(const std::string &name, ErrorCode::type_t v) {
  if (exists(name) || exists(v)) return false;
  map_v_to_name[v] = name;
  map_name_to_v[name] = v;
  return true;
}

bool
ErrorCodeMap::exists(const std::string &name) const {
  return map_name_to_v.find(name) != map_name_to_v.end();
}

bool
ErrorCodeMap::exists(ErrorCode::type_t v) const {
  return map_v_to_name.find(v) != map_v_to_name.end();
}

void
ErrorCodeMap::add_core() {
  using pair_type = decltype(map_v_to_name)::value_type;
  auto max_p = std::max_element(
      std::begin(map_v_to_name), std::end(map_v_to_name),
      [] (const pair_type &p1, const pair_type &p2) {
        return p1.first < p2.first;
      });
  auto max_v = (max_p == map_v_to_name.end()) ? 0 : (max_p->first + 1);
  // TODO(antonin): write an iterator for Core enum class instead
  for (const auto core : {Core::NoError, Core::PacketTooShort, Core::NoMatch,
                          Core::StackOutOfBounds, Core::HeaderTooShort,
                          Core::ParserTimeout, Core::ParserInvalidArgument}) {
    auto name = core_to_name(core);
    if (!exists(name)) add(name, max_v++);
  }
}

ErrorCode
ErrorCodeMap::from_name(const std::string &name) const {
  return ErrorCode(map_name_to_v.at(name));
}

ErrorCode
ErrorCodeMap::from_core(const Core &core) const {
  return ErrorCode(map_name_to_v.at(core_to_name(core)));
}

const std::string &
ErrorCodeMap::to_name(const ErrorCode &code) const {
  return map_v_to_name.at(code.v);
}

ErrorCodeMap
ErrorCodeMap::make_with_core() {
  ErrorCodeMap error_codes;
  error_codes.add_core();
  return error_codes;
}

// could be constexpr in C++14
const char *
ErrorCodeMap::core_to_name(const Core &core) {
  switch (core) {
    case Core::NoError:
      return "NoError";
    case Core::PacketTooShort:
      return "PacketTooShort";
    case Core::NoMatch:
      return "NoMatch";
    case Core::StackOutOfBounds:
      return "StackOutOfBounds";
    case Core::HeaderTooShort:
      return "HeaderTooShort";
    case Core::ParserTimeout:
      return "ParserTimeout";
    case Core::ParserInvalidArgument:
      return "ParserInvalidArgument";
  }
  // unreachable but gcc complains without it
  assert(0);
  return nullptr;
}

parser_exception_arch::parser_exception_arch(const ErrorCode &code)
    : code(code) { }

ErrorCode
parser_exception_arch::get(const ErrorCodeMap &error_codes) const {
  (void) error_codes;
  return code;
}

parser_exception_core::parser_exception_core(ErrorCodeMap::Core core)
    : core(core) { }

ErrorCode
parser_exception_core::get(const ErrorCodeMap &error_codes) const {
  return error_codes.from_core(core);
}

}  // namespace bm
