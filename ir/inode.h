/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef IR_INODE_H_
#define IR_INODE_H_

#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/rtti.h"
#include "lib/source_file.h"

namespace P4 {
class JSONGenerator;
class JSONLoader;
}  // namespace P4

namespace P4::IR {

class Node;

/// SFINAE helper to check if given class has a `static_type_name`
/// method. Definite node classes have them while interfaces do not
template <class, class = void>
struct has_static_type_name : std::false_type {};

template <class T>
struct has_static_type_name<T, std::void_t<decltype(T::static_type_name())>> : std::true_type {};

template <class T>
inline constexpr bool has_static_type_name_v = has_static_type_name<T>::value;

// node interface
class INode : public Util::IHasSourceInfo, public IHasDbPrint, public ICastable {
 public:
    virtual ~INode() {}
    virtual const Node *getNode() const = 0;
    virtual Node *getNode() = 0;
    virtual void toJSON(JSONGenerator &) const = 0;
    virtual cstring node_type_name() const = 0;
    virtual void validate() const {}

    // default checkedTo implementation for nodes: just fallback to generic ICastable method
    template <typename T>
    std::enable_if_t<!has_static_type_name_v<T>, const T *> checkedTo() const {
        return ICastable::checkedTo<T>();
    }

    // alternative checkedTo implementation that produces slightly better error message
    // due to node_type_name() / static_type_name() being available
    template <typename T>
    std::enable_if_t<has_static_type_name_v<T>, const T *> checkedTo() const {
        const auto *result = to<T>();
        BUG_CHECK(result, "Cast failed: %1% with type %2% is not a %3%.", this, node_type_name(),
                  T::static_type_name());
        return result;
    }

    DECLARE_TYPEINFO(INode);
};

}  // namespace P4::IR

#endif /* IR_INODE_H_ */
