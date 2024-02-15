#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_SPEC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_SPEC_H_

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

/* =========================================================================================
 *  PnaDpdkRegister
 * ========================================================================================= */

class PnaDpdkRegisterCondition : public TestObject {
 public:
    /// The register index.
    const IR::Expression *index;

    /// The register value.
    const IR::Expression *value;

    explicit PnaDpdkRegisterCondition(const IR::Expression *index, const IR::Expression *value);

    /// @returns the evaluated register index. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedIndex() const;

    /// @returns the evaluated condition of the register. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedValue() const;

    [[nodiscard]] const PnaDpdkRegisterCondition *evaluate(const Model &model,
                                                           bool doComplete) const override;

    [[nodiscard]] cstring getObjectName() const override;

    DECLARE_TYPEINFO(PnaDpdkRegisterCondition, TestObject);
};

/// This object tracks the list of writes that have been performed to a particular register. The
/// registerConditionList represents the pair of the index that was written, and the value that
/// was written to this index. When reading from a register, we can furl this list starting
/// from the first index into a set of nested Mux expressions (e.g., "readIndex == savedIndex ?
/// savedValue : defaultValue", where defaultValue may be another Mux expression). If the read index
/// matches with the index that was saved in this tuple, we return the value, otherwise we unroll
/// the nested MUX expressions. This implicitly handles overwrites too, as the latest writes to a
/// particular index appear the earliest in this unraveling phase..
class PnaDpdkRegisterValue : public TestObject {
 private:
    /// A new PnaDpdkRegisterValue always requires an initial value. This can be a constant or
    /// taint.
    const IR::Expression *initialValue;

    /// Each element is an API name paired with a match rule.
    std::vector<PnaDpdkRegisterCondition> registerConditions;

 public:
    explicit PnaDpdkRegisterValue(const IR::Expression *initialValue);

    [[nodiscard]] cstring getObjectName() const override;

    /// Each element is an API name paired with a match rule.
    void addRegisterCondition(PnaDpdkRegisterCondition cond);

    /// @returns the value with which this register has been initialized.
    [[nodiscard]] const IR::Expression *getInitialValue() const;

    /// @returns the current value of this register after writes have been performed according to a
    /// provided index.
    const IR::Expression *getCurrentValue(const IR::Expression *index) const;

    /// @returns the evaluated register value. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedValue() const;

    [[nodiscard]] const PnaDpdkRegisterValue *evaluate(const Model &model,
                                                       bool doComplete) const override;

    DECLARE_TYPEINFO(PnaDpdkRegisterValue, TestObject);
};

/* =========================================================================================
 *  PnaDpdkActionProfile
 * ========================================================================================= */
class PnaDpdkActionProfile : public TestObject {
 private:
    /// The list of actions associated with this profile.
    std::vector<std::pair<cstring, std::vector<ActionArg>>> actions;

    /// The associated action profile declaration.
    const IR::IDeclaration *profileDecl;

 public:
    explicit PnaDpdkActionProfile(const IR::IDeclaration *profileDecl);

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the action map of this profile.
    [[nodiscard]] const std::vector<std::pair<cstring, std::vector<ActionArg>>> *getActions() const;

    /// @returns the associated action profile declaration.
    [[nodiscard]] const IR::IDeclaration *getProfileDecl() const;

    /// @returns the current amount of actions associated with this profile.
    size_t getActionMapSize() const;

    /// Add an action (its name) and the arguments to the action map of this profile.
    void addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs);

    [[nodiscard]] const PnaDpdkActionProfile *evaluate(const Model &model,
                                                       bool doComplete) const override;

    DECLARE_TYPEINFO(PnaDpdkActionProfile, TestObject);
};

/* =========================================================================================
 *  ActionSelector
 * ========================================================================================= */
class PnaDpdkActionSelector : public TestObject {
 private:
    /// The associated action selector declaration.
    const IR::IDeclaration *selectorDecl;

    /// The associated action profile.
    const PnaDpdkActionProfile *actionProfile;

 public:
    explicit PnaDpdkActionSelector(const IR::IDeclaration *selectorDecl,
                                   const PnaDpdkActionProfile *actionProfile);

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the associated action selector declaration.
    [[nodiscard]] const IR::IDeclaration *getSelectorDecl() const;

    /// @returns the associated action profile.
    [[nodiscard]] const PnaDpdkActionProfile *getActionProfile() const;

    [[nodiscard]] const PnaDpdkActionSelector *evaluate(const Model &model,
                                                        bool doComplete) const override;

    DECLARE_TYPEINFO(PnaDpdkActionSelector, TestObject);
};

/* =========================================================================================
 *  MetadataCollection
 * ========================================================================================= */
class MetadataCollection : public TestObject {
 private:
    std::map<cstring, const IR::Literal *> metadataFields;

 public:
    MetadataCollection();

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const MetadataCollection *evaluate(const Model & /*model*/,
                                                     bool doComplete) const override;

    /// @returns the clone port expression.
    [[nodiscard]] const std::map<cstring, const IR::Literal *> &getMetadataFields() const;

    void addMetaDataField(cstring name, const IR::Literal *metadataField);

    const IR::Literal *getMetadataField(cstring name);

    DECLARE_TYPEINFO(MetadataCollection, TestObject);
};

/* =========================================================================================
 * Table Key Match Types
 * ========================================================================================= */

class Optional : public TableMatch {
 private:
    /// The value the key is matched with.
    const IR::Expression *value;

    /// Whether to add this optional match as an exact match.
    bool addMatch;

 public:
    explicit Optional(const IR::KeyElement *key, const IR::Expression *value, bool addMatch);

    const Optional *evaluate(const Model &model, bool doComplete) const override;

    cstring getObjectName() const override;

    /// @returns the match value. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant *getEvaluatedValue() const;

    /// @returns whether to add this optional match as an exact match.
    bool addAsExactMatch() const;

    DECLARE_TYPEINFO(Optional, TableMatch);
};

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

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_SPEC_H_ */
