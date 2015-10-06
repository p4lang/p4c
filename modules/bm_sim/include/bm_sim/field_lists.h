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

#include <vector>

#include "phv.h"

class FieldList
{
public:
  typedef std::vector<std::pair<header_id_t, int> >::iterator iterator;
  typedef std::vector<std::pair<header_id_t, int> >::const_iterator const_iterator;
  typedef std::vector<std::pair<header_id_t, int> >::reference reference;
  typedef std::vector<std::pair<header_id_t, int> >::const_reference const_reference;
  typedef size_t size_type;

public:
  void push_back_field(header_id_t header, int field_offset) {
    fields.push_back(std::make_pair(header, field_offset));
  }

  // iterators
  iterator begin() { return fields.begin(); }

  const_iterator begin() const { return fields.begin(); }

  iterator end() { return fields.end(); }

  const_iterator end() const { return fields.end(); }

private:
  std::vector<std::pair<header_id_t, int> > fields;
};
