/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _bson_h_
#define _bson_h_

#include <type_traits>

#include "backends/tofino/bf-asm/json.h"

namespace json {

template <class T>
struct bson_wrap {
    T &o;
    bson_wrap(T &o) : o(o) {}
    template <class U>
    bson_wrap(U &o) : o(o) {}
};

template <class T>
bson_wrap<T> binary(T &o) {
    return bson_wrap<T>(o);
}

std::istream &operator>>(std::istream &in, bson_wrap<std::unique_ptr<obj>> json);
std::istream &operator>>(std::istream &in, bson_wrap<json::vector> json);
std::istream &operator>>(std::istream &in, bson_wrap<json::map> json);
inline std::istream &operator>>(std::istream &in, bson_wrap<obj *> json) {
    std::unique_ptr<obj> p;
    in >> binary(p);
    if (in) json.o = p.release();
    return in;
}

std::ostream &operator<<(std::ostream &out, bson_wrap<const vector>);
std::ostream &operator<<(std::ostream &out, bson_wrap<const map>);
std::ostream &operator<<(std::ostream &out, bson_wrap<const obj> json);
inline std::ostream &operator<<(std::ostream &out, bson_wrap<vector> json) {
    return operator<<(out, bson_wrap<const vector>(json.o));
}
inline std::ostream &operator<<(std::ostream &out, bson_wrap<map> json) {
    return operator<<(out, bson_wrap<const map>(json.o));
}
inline std::ostream &operator<<(std::ostream &out, bson_wrap<obj> json) {
    return operator<<(out, bson_wrap<const obj>(json.o));
}
inline std::ostream &operator<<(std::ostream &out, bson_wrap<const obj *> json) {
    return out << binary(*json.o);
}
inline std::ostream &operator<<(std::ostream &out, bson_wrap<obj *> json) {
    return out << binary(*json.o);
}
inline std::ostream &operator<<(std::ostream &out, bson_wrap<const std::unique_ptr<obj>> json) {
    return out << binary(*json.o.get());
}

}  // end namespace json

#endif /* _bson_h_ */
