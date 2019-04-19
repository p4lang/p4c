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

#ifndef _FRONTENDS_P4_MOVECONSTRUCTORS_H_
#define _FRONTENDS_P4_MOVECONSTRUCTORS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/**
 * This pass converts constructor call expressions that appear
 * within the bodies of P4Parser and P4Control blocks
 * into Declaration_Instance. This is needed for implementing
 * copy-in/copy-out in inlining, since
 * constructed objects do not have assignment operations.
 *
 * For example:
 * extern T {}
 * control c()(T t) {  apply { ... } }
 * control d() {
 *    c(T()) cinst;
 *    apply { ... }
 * }
 * is converted to
 * extern T {}
 * control c()(T t) {  apply { ... } }
 * control d() {
 *    T() tmp;
 *    c(tmp) cinst;
 *    apply { ... }
 * }
 *
 * @pre None
 * @post No constructor call expression in P4Parser and P4Control
 *       constructor parameters.
 *
 */
class MoveConstructors : public PassManager {
 public:
    explicit MoveConstructors(ReferenceMap* refMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_MOVECONSTRUCTORS_H_ */
