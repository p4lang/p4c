/* Copyright 2017 Cisco Systems, Inc.
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
 * Andy Fingerhut (jafinger@cisco.com)
 *
 */

#ifndef BM_BM_SIM_SOURCE_INFO_H_
#define BM_BM_SIM_SOURCE_INFO_H_

//! @file source_info.h
//! Stores source location information about some objects, as read from
//! the JSON file produced by the compiler.

#include <string>

namespace bm {

class SourceInfo {
 public:
  SourceInfo(std::string filename, unsigned int line, unsigned int column,
             std::string source_fragment)
    : filename(filename), line(line), column(column),
      source_fragment(source_fragment) {
    init_to_string();
  }

  std::string get_filename() const { return filename; }
  unsigned int get_line() const { return line; }
  unsigned int get_column() const { return column; }
  std::string get_source_fragment() const { return source_fragment; }
  std::string to_string() const { return string_representation; }

 private:
  std::string filename;
  unsigned int line;
  unsigned int column;
  std::string source_fragment;
  std::string string_representation;

  void init_to_string();
};

}  // namespace bm

#endif  // BM_BM_SIM_SOURCE_INFO_H_
