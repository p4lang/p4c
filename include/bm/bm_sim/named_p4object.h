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

//! @file named_p4object.h

#ifndef BM_BM_SIM_NAMED_P4OBJECT_H_
#define BM_BM_SIM_NAMED_P4OBJECT_H_

#include <string>

namespace bm {

typedef int p4object_id_t;

//! NamedP4Object is used as a base class for all the bmv2 classes with are used
//! to represent named P4 objects (e.g. Parser for P4 `parser` objects). It
//! just stores the name of the P4 instance and a compiler-provided id, which is
//! different from the id of other objects of the same class.
class NamedP4Object {
 public:
  NamedP4Object(const std::string &name, p4object_id_t id)
    : name(name), id(id) {}

  virtual ~NamedP4Object() { }

  //! Get the name of the P4 instance
  const std::string &get_name() const { return name; }

  //! Get the compiler-provided id
  p4object_id_t get_id() const { return id; }

 protected:
  const std::string name;
  p4object_id_t id;
};

}  // namespace bm

#endif  // BM_BM_SIM_NAMED_P4OBJECT_H_
