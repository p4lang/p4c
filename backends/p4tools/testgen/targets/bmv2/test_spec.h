#ifndef TESTGEN_TARGETS_BMV2_TEST_SPEC_H_
#define TESTGEN_TARGETS_BMV2_TEST_SPEC_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen/lib/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/* =========================================================================================
 *  Uninterpreted Bmv2Register
 * ========================================================================================= */

class Bmv2RegisterCondition : public TestObject {
 public:
    /// Each element is an API name paired with a match rule.
    const IR::Expression* index;

    const IR::Expression* value;

    explicit Bmv2RegisterCondition(const IR::Expression* index, const IR::Expression* value);

    const IR::Constant* getEvaluatedIndex() const {
        auto constant = index->to<IR::Constant>();
        BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
                  getObjectName());
        return constant;
    }

    const IR::Constant* getEvaluatedValue() const {
        auto constant = value->to<IR::Constant>();
        BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
                  getObjectName());
        return constant;
    }

    const Bmv2RegisterCondition* evaluate(const Model& model) const override {
        auto evaluatedIndex = model.evaluate(index);
        auto evaluatedValue = model.evaluate(value);
        return new Bmv2RegisterCondition(evaluatedIndex, evaluatedValue);
    }

    cstring getObjectName() const override { return "Bmv2RegisterCondition"; }
};

/// This object tracks the list of writes that have been performed to a particular register. The
/// registerConditionList represents the pair of the index that was written, and the value that
/// was written to this index. When reading from a register, we can ravel this list starting
/// from the first index into a set of nested Mux expressions (e.g., "readIndex == savedIndex ?
/// savedValue : defaultValue", where defaultValue may be another Mux expression). If the read index
/// matches with the index that was saved in this tuple, we return the value, otherwise we unroll
/// the nested MUX expressions. This implicitly handles overwrites too, as the latest writes to a
/// particular index appear the earliest in this unraveling phase..
class Bmv2RegisterValue : public TestObject {
 private:
    /// A new Bmv2RegisterValue always requires an initial value. This can be a constant or taint.
    const IR::Expression* initialValue;

    /// Each element is an API name paired with a match rule.
    std::vector<Bmv2RegisterCondition> registerConditions;

 public:
    explicit Bmv2RegisterValue(const IR::Expression* initialValue);

    cstring getObjectName() const override;

    /// Each element is an API name paired with a match rule.
    void addRegisterCondition(Bmv2RegisterCondition cond);

    /// @returns the value with which this register has been initialized.
    const IR::Expression* getInitialValue() const;

    /// @returns the current value of this register after writes have been performed according to a
    /// provided index.
    const IR::Expression* getCurrentValue(const IR::Expression* index) const;

    const IR::Constant* getEvaluatedValue() const {
        auto constant = initialValue->to<IR::Constant>();
        BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
                  getObjectName());
        return constant;
    }

    const Bmv2RegisterValue* evaluate(const Model& model) const override {
        auto evaluatedValue = model.evaluate(initialValue);
        auto evaluatedRegisterValue = new Bmv2RegisterValue(evaluatedValue);
        std::vector<ActionArg> evaluatedConditions;
        for (auto cond : registerConditions) {
            evaluatedRegisterValue->addRegisterCondition(*cond.evaluate(model));
        }
        return evaluatedRegisterValue;
    }
};

/* =========================================================================================
 *  Bmv2_V1ModelActionProfile
 * ========================================================================================= */
class Bmv2_V1ModelActionProfile : public TestObject {
 private:
    /// The list of actions associated with this profile.
    std::vector<std::pair<cstring, std::vector<ActionArg>>> actions;

    /// The associated action profile declaration.
    const IR::IDeclaration* profileDecl;

 public:
    explicit Bmv2_V1ModelActionProfile(const IR::IDeclaration* profileDecl);

    cstring getObjectName() const override;

    /// @returns the action map of this profile.
    const std::vector<std::pair<cstring, std::vector<ActionArg>>>* getActions() const;

    /// @returns the associated action profile declaration.
    const IR::IDeclaration* getProfileDecl() const;

    /// @returns the current amount of actions associated with this profile.
    size_t getActionMapSize() const;

    /// Add an action (its name) and the arguments to the action map of this profile.
    void addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs);

    const Bmv2_V1ModelActionProfile* evaluate(const Model& model) const override {
        auto profile = new Bmv2_V1ModelActionProfile(profileDecl);
        for (auto actionTuple : actions) {
            auto actionArgs = actionTuple.second;
            std::vector<ActionArg> evaluatedArgs;
            for (auto actionArg : actionArgs) {
                evaluatedArgs.emplace_back(*actionArg.evaluate(model));
            }
            profile->addToActionMap(actionTuple.first, evaluatedArgs);
        }
        return profile;
    }
};

/* =========================================================================================
 *  ActionSelector
 * ========================================================================================= */
class Bmv2_V1ModelActionSelector : public TestObject {
 private:
    /// The associated action selector declaration.
    const IR::IDeclaration* selectorDecl;

    /// The associated action profile.
    const ActionProfile* actionProfile;

 public:
    explicit Bmv2_V1ModelActionSelector(const IR::IDeclaration* selectorDecl,
                                        const ActionProfile* actionProfile);

    cstring getObjectName() const override;

    /// @returns the associated action selector declaration.
    const IR::IDeclaration* getSelectorDecl() const;

    /// @returns the associated action profile.
    const ActionProfile* getActionProfile() const;

    const Bmv2_V1ModelActionSelector* evaluate(const Model& model) const override {
        auto evaluatedProfile = actionProfile->evaluate(model);
        return new Bmv2_V1ModelActionSelector(selectorDecl, evaluatedProfile);
    }
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_TEST_SPEC_H_ */
