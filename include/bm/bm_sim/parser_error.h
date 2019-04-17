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

//! @file parser_error.h

#ifndef BM_BM_SIM_PARSER_ERROR_H_
#define BM_BM_SIM_PARSER_ERROR_H_

#include <exception>
#include <limits>
#include <string>
#include <unordered_map>

namespace bm {

//! Used to represent an error code (used in the parser) internally. This is
//! really just a wrapper around an integral type.
class ErrorCode {
  friend class ErrorCodeMap;
 public:
  using type_t = int;

  explicit ErrorCode(type_t v)
      : v(v) { }

  type_t get() { return v; }

  //! Equality operator
  bool operator==(const ErrorCode &other) const {
    return (v == other.v);
  }

  //! Inequality operator
  bool operator!=(const ErrorCode &other) const {
    return !(*this == other);
  }

  //! An invalid ErrorCode, with an integral value distinct from all other
  //! P4-specified error codes.
  static ErrorCode make_invalid() { return ErrorCode(INVALID_V); }

 private:
  type_t v;

  static constexpr type_t INVALID_V = std::numeric_limits<type_t>::max();
};

//! A bi-directional map between error codes and their P4 names.
class ErrorCodeMap {
 public:
  //! The core erros, as per core.p4.
  enum class Core {
    //! No error raised in parser
    NoError,
    //! Not enough bits in packet for extract or lookahead
    PacketTooShort,
    //! Match statement has no matches (unused for now)
    NoMatch,
    //! Reference to invalid element of a header stack (partial support)
    StackOutOfBounds,
    //! Extracting too many bits into a varbit field (unused for now)
    HeaderTooShort,
    //! Parser execution time limit exceeded (unused for now)
    ParserTimeout,
    //! Parser operation was called with a value not supported by the
    //! implementation.
    ParserInvalidArgument
  };

  bool add(const std::string &name, ErrorCode::type_t v);
  bool exists(const std::string &name) const;
  bool exists(ErrorCode::type_t v) const;
  // Add the core error codes to the map, providing they are not already
  // present. If they need to be added, we make sure to use different integral
  // values than for the existing codes.
  void add_core();

  //! Retrieve an error code from a P4 error name. Will throw std::out_of_range
  //! exception if name does not exist.
  ErrorCode from_name(const std::string &name) const;
  //! Retrieve an error code for one of the core errors.
  ErrorCode from_core(const Core &core) const;
  //! Retrieve an error code's name. Will throw std::out_of_range exception if
  //! the error code is not valid.
  const std::string &to_name(const ErrorCode &code) const;

  static ErrorCodeMap make_with_core();

 private:
  static const char *core_to_name(const Core &core);

  std::unordered_map<ErrorCode::type_t, std::string> map_v_to_name{};
  std::unordered_map<std::string, ErrorCode::type_t> map_name_to_v{};
};

class parser_exception : public std::exception {
 public:
  virtual ~parser_exception() { }

  virtual ErrorCode get(const ErrorCodeMap &error_codes) const = 0;
};

class parser_exception_arch : public parser_exception {
 public:
  explicit parser_exception_arch(const ErrorCode &code);

  ErrorCode get(const ErrorCodeMap &error_codes) const override;

 private:
  const ErrorCode code;
};

class parser_exception_core : public parser_exception {
 public:
  explicit parser_exception_core(ErrorCodeMap::Core core);

  ErrorCode get(const ErrorCodeMap &error_codes) const override;

 private:
  ErrorCodeMap::Core core;
};

}  // namespace bm

#endif  // BM_BM_SIM_PARSER_ERROR_H_
