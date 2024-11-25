/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_SPEC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_SPEC_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/* =========================================================================================
 *  IndexExpression
 * ========================================================================================= */
/// Associates an expression with a particular index.
/// This object is used by all extern object that depend on a particular index to retrieve a value.
/// Examples are registers, meters, or counters.
class IndexExpression : public TestObject {
 private:
    /// The index of the expression.
    const IR::Expression *index;

    /// The value of the expression.
    const IR::Expression *value;

 public:
    explicit IndexExpression(const IR::Expression *index, const IR::Expression *value);

    /// @returns the evaluated expression index. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedIndex() const;

    /// @returns the evaluated condition of the expression. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedValue() const;

    /// @returns the index stored in the index expression.
    [[nodiscard]] const IR::Expression *getIndex() const;

    /// @returns the value stored in the index expression.
    [[nodiscard]] const IR::Expression *getValue() const;

    [[nodiscard]] const IndexExpression *evaluate(const Model &model,
                                                  bool doComplete) const override;

    [[nodiscard]] cstring getObjectName() const override;

    DECLARE_TYPEINFO(IndexExpression, TestObject);
};

/* =========================================================================================
 *  IndexMap
 * ========================================================================================= */
/// Readable and writable symbolic map, which maps indices to particular values.
class IndexMap : public TestObject {
 protected:
    /// A new IndexMap always requires an initial value. This can be a constant or taint.
    const IR::Expression *initialValue;

    /// Each element is an API name paired with a match rule.
    std::vector<IndexExpression> indexConditions;

 public:
    explicit IndexMap(const IR::Expression *initialValue);

    /// Write @param value to @param index.
    void writeToIndex(const IR::Expression *index, const IR::Expression *value);

    /// @returns the value with which this register has been initialized.
    [[nodiscard]] const IR::Expression *getInitialValue() const;

    /// @returns the current value of this register after writes have been performed according to a
    /// provided index.
    [[nodiscard]] const IR::Expression *getValueAtIndex(const IR::Expression *index) const;

    /// @returns the evaluated register value. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Expression *getEvaluatedInitialValue() const;

    /// Return the "writes" to this index map as a
    [[nodiscard]] std::map<big_int, std::pair<int, const IR::Constant *>> unravelMap() const;

    DECLARE_TYPEINFO(IndexMap, TestObject);
};

/* =========================================================================================
 *  TofinoRegisterValue
 * ========================================================================================= */
/// This object tracks the list of writes that have been performed to a particular register. The
/// IndexMap represents the pair of the index that was written, and the value that was
/// written to this index. When reading from a register, we can ravel this list starting from the
/// first index into a set of nested Mux expressions (e.g., "readIndex == savedIndex ? savedValue :
/// defaultValue", where defaultValue may be another Mux expression). If the read index matches with
/// the index that was saved in this tuple, we return the value, otherwise we unroll the nested MUX
/// expressions. This implicitly handles overwrites too, as the latest writes to a particular
/// index appear the earliest in this unraveling phase..
class TofinoRegisterValue : public IndexMap {
 private:
    /// The register declaration, also contains the control plane name.
    const IR::Declaration_Instance *decl;

    /// The index this register is initialized at.
    const IR::Expression *initialIndex;

 public:
    explicit TofinoRegisterValue(const IR::Declaration_Instance *decl,
                                 const IR::Expression *initialValue,
                                 const IR::Expression *initialIndex);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const TofinoRegisterValue *evaluate(const Model &model,
                                                      bool doComplete) const override;

    /// @returns the declaration of this register.
    [[nodiscard]] const IR::Declaration_Instance *getRegisterDeclaration() const;

    /// @returns the initial index this register is initialized with.
    [[nodiscard]] const IR::Constant *getEvaluatedInitialIndex() const;

    DECLARE_TYPEINFO(TofinoRegisterValue, TestObject);
};

/* =========================================================================================
 *  TofinoDirectRegisterValue
 * ========================================================================================= */
/// DirectRegisters do not have an index and are attached to tables.
class TofinoDirectRegisterValue : public TestObject {
 private:
    /// The table this direct register is attached to.
    const IR::Expression *initialValue;

    /// The register declaration, also contains the control plane name.
    const IR::Declaration_Instance *decl;

    /// The table this direct register is attached to.
    const IR::P4Table *table;

 public:
    explicit TofinoDirectRegisterValue(const IR::Declaration_Instance *decl,
                                       const IR::Expression *initialValue,
                                       const IR::P4Table *table);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const TofinoDirectRegisterValue *evaluate(const Model &model,
                                                            bool doComplete) const override;

    /// @returns the declaration of this register.
    [[nodiscard]] const IR::Declaration_Instance *getRegisterDeclaration() const;

    /// @returns the initial value this register is initialized with.
    [[nodiscard]] const IR::Expression *getInitialValue() const;

    /// @returns the table this direct register is attached to.
    [[nodiscard]] const IR::P4Table *getRegisterTable() const;

    /// @returns the evaluated initial value this register is initialized with.
    [[nodiscard]] const IR::Expression *getEvaluatedInitialValue() const;

    DECLARE_TYPEINFO(TofinoDirectRegisterValue, TestObject);
};

/* =========================================================================================
 *  TofinoRegisterParam
 * ========================================================================================= */
/// Tofino register params have one default value and are read only.
class TofinoRegisterParam : public TestObject {
 private:
    /// The register param declaration, also contains the control plane name.
    const IR::Declaration_Instance *decl;

    /// The initial value of this register parameter.
    const IR::Expression *initialValue;

 public:
    explicit TofinoRegisterParam(const IR::Declaration_Instance *decl,
                                 const IR::Expression *initialValue);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const TofinoRegisterParam *evaluate(const Model &model,
                                                      bool doComplete) const override;
    /// @returns the initial value this register is supposed to be initialized with.
    [[nodiscard]] const IR::Expression *getInitialValue() const;

    /// @returns the declaration of this register parameter.
    [[nodiscard]] const IR::Declaration_Instance *getRegisterParamDeclaration() const;

    /// @returns the evaluated initial value this register is supposed to be initialized with.
    [[nodiscard]] const IR::Constant *getEvaluatedInitialValue() const;

    DECLARE_TYPEINFO(TofinoRegisterParam, TestObject);
};

/* =========================================================================================
 *  TofinoActionProfile
 * ========================================================================================= */
class TofinoActionProfile : public TestObject {
 private:
    /// The list of actions associated with this profile.
    std::vector<std::pair<cstring, std::vector<ActionArg>>> actions;

    /// The associated action profile declaration.
    const IR::IDeclaration *profileDecl;

 public:
    explicit TofinoActionProfile(const IR::IDeclaration *profileDecl);

    cstring getObjectName() const override;

    /// @returns the action map of this profile.
    const std::vector<std::pair<cstring, std::vector<ActionArg>>> *getActions() const;

    /// @returns the associated action profile declaration.
    const IR::IDeclaration *getProfileDecl() const;

    /// @returns the current amount of actions associated with this profile.
    size_t getActionMapSize() const;

    /// Add an action (its name) and the arguments to the action map of this profile.
    void addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs);

    const TofinoActionProfile *evaluate(const Model &model, bool doComplete) const override;

    DECLARE_TYPEINFO(TofinoActionProfile, TestObject);
};

/* =========================================================================================
 *  TofinoActionSelector
 * ========================================================================================= */
class TofinoActionSelector : public TestObject {
 private:
    /// The associated action selector declaration.
    const IR::IDeclaration *selectorDecl;

    /// The associated action profile.
    const TofinoActionProfile *actionProfile;

 public:
    explicit TofinoActionSelector(const IR::IDeclaration *selectorDecl,
                                  const TofinoActionProfile *actionProfile);

    cstring getObjectName() const override;

    /// @returns the associated action selector declaration.
    const IR::IDeclaration *getSelectorDecl() const;

    /// @returns the associated action profile.
    const TofinoActionProfile *getActionProfile() const;

    const TofinoActionSelector *evaluate(const Model &model, bool doComplete) const override;

    DECLARE_TYPEINFO(TofinoActionSelector, TestObject);
};

/* =========================================================================================
 * Table Key Match Types
 * ========================================================================================= */

class Range : public TableMatch {
 private:
    /// The inclusive start of the range.
    const IR::Expression *low;

    /// The inclusive end of the range.
    const IR::Expression *high;

 public:
    explicit Range(const IR::KeyElement *key, const IR::Expression *low,
                   const IR::Expression *high);

    const Range *evaluate(const Model &model, bool doComplete) const override;

    cstring getObjectName() const override;

    /// @returns the inclusive start of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant *getEvaluatedLow() const;

    /// @returns the inclusive end of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant *getEvaluatedHigh() const;

    DECLARE_TYPEINFO(Range, TableMatch);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_TEST_SPEC_H_ */
