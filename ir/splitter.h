/*
Copyright 2025-present Altera Corporation.

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

#ifndef IR_SPLITTER_H_
#define IR_SPLITTER_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class NameGenerator;
class TypeMap;

template <typename Node>
struct SplitResult {
    const Node *before = nullptr;
    const Node *after = nullptr;
    std::vector<const IR::Declaration *> hoistedDeclarations;
};

/// @brief  Split @p stat so that on every control-flow path all the statements up to the first one
/// matching @p predicate are in SplitResult::before, while the rest (starting from the matching
/// statemet) is in SplitResult::after.
///
/// @pre @p stat must not contain P4::IR::LoopStatement (loops must be unrolled before).
/// @pre All variable declarations have unique names.
/// @pre No non-standard control flow blocks exist in the IR of @p stat (only if and switch).
///
/// @note Fresh variables are introduced to save all if conditions and switch selectors. This is to
/// ensure the right branch is triggered even if code is inserted between the split points.
/// Furthemore, declarations that would become invisible in the "after section" are hoisted. No
/// other provisions are made to isolate side effects that may be inserted between the split code
/// fragments.
///
/// @note No inspection is done for the called object of IR::MA::MethodCallStatement (except that @p
/// predicate is applied to them as for any other statement). Therefore, called functions/actions
/// are not recursively split.
///
/// @code{.p4}
/// a = a + 4
/// if (b > 5) {
///     bit<3> v = c + 2;
///     t1.apply();
///     d = 8 + v;
/// } else {
///     if (e == 4) {
///         d = 1;
///         t2.apply();
///         c = 2
///     }
/// }
/// @endcode
///
/// If we split this according to calls to table.apply, we get:
///
/// Before:
/// @code{.p4}
/// a = a + 4
/// cond_1 = b > 5;
/// if (cond_1) {
///     v = c + 2;
/// } else {
///     cond_2 = e == 4;
///     if (cond_2) {
///         d = 1;
///     }
/// }
/// @endcode
///
/// After:
/// @code{.p4}
/// if (cond_1) {
///     t1.apply();
///     d = 8 + v;
/// } else {
///     if (cond_2) {
///         t2.apply();
///         c = 2
///     }
/// }
/// @endcode
///
/// Hoisted declarations:
/// @code{.p4}
/// bool cond_1;
/// bool cond_2;
/// bit<3> v;
/// @endcode
///
///
/// @param stat      The statement to split.
/// @param predicate Predicate over statements. Returns true for split point.
/// @param nameGen   A name generator valid for the P4 program containing @p stat.
/// @param typeMap   A P4::TypeMap valid for the program, or nullptr. In case nullptr is passed,
///                  types must be encoded in the IR, or no P4::IR::SwitchStatement can be split.
/// @return The after node is empty (`nullptr`) if no splitting occurred.
/// SplitResult::hoistedDeclarations will contain declarations that have to be made available for
/// the two statements to work.
SplitResult<IR::Statement> splitStatementBefore(
    const IR::Statement *stat,
    std::function<bool(const IR::Statement *, const P4::Visitor_Context *)> predicate,
    P4::NameGenerator &nameGen, P4::TypeMap *typeMap = nullptr);

}  // namespace P4

#endif  // IR_SPLITTER_H_
