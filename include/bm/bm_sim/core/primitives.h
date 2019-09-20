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

#ifndef BM_BM_SIM_CORE_PRIMITIVES_H_
#define BM_BM_SIM_CORE_PRIMITIVES_H_

#include <bm/bm_sim/actions.h>

#include <string>
#include <vector>

namespace bm {

namespace core {

struct assign : public ActionPrimitive<Data &, const Data &> {
  void operator ()(Data &dst, const Data &src) {
    dst.set(src);
  }
};

struct assign_VL : public ActionPrimitive<Field &, const Field &> {
  void operator ()(Field &dst, const Field &src) {
    dst.assign_VL(src);
  }
};

struct assign_header : public ActionPrimitive<Header &, const Header &> {
  void operator ()(Header &dst, const Header &src);
};

struct assign_union
    : public ActionPrimitive<HeaderUnion &, const HeaderUnion &> {
  void operator ()(HeaderUnion &dst, const HeaderUnion &src);
};

struct assign_header_stack
    : public ActionPrimitive<HeaderStack &, const HeaderStack &> {
  void operator ()(HeaderStack &dst, const HeaderStack &src);
};

struct assign_union_stack
    : public ActionPrimitive<HeaderUnionStack &, const HeaderUnionStack &> {
  void operator ()(HeaderUnionStack &dst, const HeaderUnionStack &src);
};

struct push : public ActionPrimitive<StackIface &, const Data &> {
  void operator ()(StackIface &stack, const Data &num);
};

struct pop : public ActionPrimitive<StackIface &, const Data &> {
  void operator ()(StackIface &stack, const Data &num);
};

struct assert_ : public ActionPrimitive<const Data &> {
  void operator ()(const Data &src);
};

struct assume_ : public ActionPrimitive<const Data &> {
  void operator ()(const Data &src);
};

class exit_ : public ActionPrimitive<> {
  void operator ()();
};

struct log_msg : public ActionPrimitive<const std::string &,
                                        const std::vector<Data> > {
  void operator ()(const std::string &format,
                   const std::vector<Data> data_vector);
};

}  // namespace core

}  // namespace bm

#endif  // BM_BM_SIM_CORE_PRIMITIVES_H_
