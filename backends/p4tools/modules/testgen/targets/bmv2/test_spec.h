#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_SPEC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_SPEC_H_

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/rtti.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"

namespace P4Tools::P4Testgen::Bmv2 {

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
    [[nodiscard]] const IR::Constant *getEvaluatedInitialValue() const;

    /// Return the "writes" to this index map as a
    [[nodiscard]] std::map<big_int, std::pair<int, const IR::Constant *>> unravelMap() const;

    DECLARE_TYPEINFO(IndexMap, TestObject);
};

/* =========================================================================================
 *  Bmv2V1ModelRegisterValue
 * ========================================================================================= */
/// This object tracks the list of writes that have been performed to a particular register. The
/// registerConditionList represents the pair of the index that was written, and the value that
/// was written to this index. When reading from a register, we can furl this list starting
/// from the first index into a set of nested Mux expressions (e.g., "readIndex == savedIndex ?
/// savedValue : defaultValue", where defaultValue may be another Mux expression). If the read
/// index matches with the index that was saved in this tuple, we return the value, otherwise we
/// unroll the nested MUX expressions. This implicitly handles overwrites too, as the latest
/// writes to a particular index appear the earliest in this unraveling phase..
class Bmv2V1ModelRegisterValue : public IndexMap {
 public:
    explicit Bmv2V1ModelRegisterValue(const IR::Expression *initialValue);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const Bmv2V1ModelRegisterValue *evaluate(const Model &model,
                                                           bool doComplete) const override;

    DECLARE_TYPEINFO(Bmv2V1ModelRegisterValue, IndexMap);
};

/* =========================================================================================
 *  Bmv2V1ModelMeterValue
 * ========================================================================================= */

class Bmv2V1ModelMeterValue : public IndexMap {
 private:
    /// Whether the meter is a direct meter.
    bool isDirect;

 public:
    explicit Bmv2V1ModelMeterValue(const IR::Expression *initialValue, bool isDirect);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const Bmv2V1ModelMeterValue *evaluate(const Model &model,
                                                        bool doComplete) const override;

    /// @returns whether the meter associated with this meter value object is a direct meter.
    [[nodiscard]] bool isDirectMeter() const;

    DECLARE_TYPEINFO(Bmv2V1ModelMeterValue, IndexMap);
};

/* =========================================================================================
 *  Bmv2V1ModelActionProfile
 * ========================================================================================= */
class Bmv2V1ModelActionProfile : public TestObject {
 private:
    /// The list of actions associated with this profile.
    std::vector<std::pair<cstring, std::vector<ActionArg>>> actions;

    /// The associated action profile declaration.
    const IR::IDeclaration *profileDecl;

 public:
    explicit Bmv2V1ModelActionProfile(const IR::IDeclaration *profileDecl);

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the action map of this profile.
    [[nodiscard]] const std::vector<std::pair<cstring, std::vector<ActionArg>>> *getActions() const;

    /// @returns the associated action profile declaration.
    [[nodiscard]] const IR::IDeclaration *getProfileDecl() const;

    /// @returns the current amount of actions associated with this profile.
    [[nodiscard]] size_t getActionMapSize() const;

    /// Add an action (its name) and the arguments to the action map of this profile.
    void addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs);

    [[nodiscard]] const Bmv2V1ModelActionProfile *evaluate(const Model &model,
                                                           bool doComplete) const override;

    DECLARE_TYPEINFO(Bmv2V1ModelActionProfile, TestObject);
};

/* =========================================================================================
 *  ActionSelector
 * ========================================================================================= */
class Bmv2V1ModelActionSelector : public TestObject {
 private:
    /// The associated action selector declaration.
    const IR::IDeclaration *selectorDecl;

    /// The associated action profile.
    const Bmv2V1ModelActionProfile *actionProfile;

 public:
    explicit Bmv2V1ModelActionSelector(const IR::IDeclaration *selectorDecl,
                                       const Bmv2V1ModelActionProfile *actionProfile);

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the associated action selector declaration.
    [[nodiscard]] const IR::IDeclaration *getSelectorDecl() const;

    /// @returns the associated action profile.
    [[nodiscard]] const Bmv2V1ModelActionProfile *getActionProfile() const;

    [[nodiscard]] const Bmv2V1ModelActionSelector *evaluate(const Model &model,
                                                            bool doComplete) const override;

    DECLARE_TYPEINFO(Bmv2V1ModelActionSelector, TestObject);
};

/* =========================================================================================
 *  Bmv2V1ModelCloneInfo
 * ========================================================================================= */
class Bmv2V1ModelCloneInfo : public TestObject {
 private:
    /// The session ID associated with this clone information.
    const IR::Expression *sessionId;

    /// The type of clone associated with this object.
    BMv2Constants::CloneType cloneType;

    /// The state at the point of time time this object was created.
    std::reference_wrapper<const ExecutionState> clonedState;

    /// Whether to preserve a particular field list of metadata. This is optional.
    std::optional<int> preserveIndex;

 public:
    explicit Bmv2V1ModelCloneInfo(const IR::Expression *sessionId,
                                  BMv2Constants::CloneType cloneType,
                                  const ExecutionState &clonedState,
                                  std::optional<int> preserveIndex);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const Bmv2V1ModelCloneInfo *evaluate(const Model &model,
                                                       bool doComplete) const override;

    /// @returns the associated session ID with this cloned packet.
    [[nodiscard]] const IR::Expression *getSessionId() const;

    /// @returns the type of clone.
    [[nodiscard]] BMv2Constants::CloneType getCloneType() const;

    /// @returns the index marking the field list to be preserved when cloning. Optional.
    [[nodiscard]] std::optional<int> getPreserveIndex() const;

    /// @returns the state that was cloned at the time of generation of this object.
    [[nodiscard]] const ExecutionState &getClonedState() const;

    DECLARE_TYPEINFO(Bmv2V1ModelCloneInfo, TestObject);
};

/* =========================================================================================
 *  Bmv2V1ModelCloneSpec
 * ========================================================================================= */
class Bmv2V1ModelCloneSpec : public TestObject {
 private:
    /// The session ID associated with this clone information.
    const IR::Expression *sessionId;

    /// The cloned packet will be emitted on this port.
    const IR::Expression *clonePort;

    /// Whether this clone information is associated with the cloned packet (true) or the
    /// regular packet (false).
    bool isClone;

 public:
    explicit Bmv2V1ModelCloneSpec(const IR::Expression *sessionId, const IR::Expression *clonePort,
                                  bool isClone);

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const Bmv2V1ModelCloneSpec *evaluate(const Model &model,
                                                       bool doComplete) const override;

    /// @returns the associated session ID with this cloned packet.
    [[nodiscard]] const IR::Expression *getSessionId() const;

    /// @returns the clone port expression.
    [[nodiscard]] const IR::Expression *getClonePort() const;

    /// @returns information whether we are dealing with the packet clone or the real output
    /// packet.
    [[nodiscard]] bool isClonedPacket() const;

    /// @returns the evaluated clone port. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedClonePort() const;

    /// @returns the evaluated session id. This means it must be a constant.
    /// The function will throw a bug if this is not the case.
    [[nodiscard]] const IR::Constant *getEvaluatedSessionId() const;

    DECLARE_TYPEINFO(Bmv2V1ModelCloneSpec, TestObject);
};

/* =========================================================================================
 *  MetadataCollection
 * ========================================================================================= */
class MetadataCollection : public TestObject {
 private:
    /// A list of metadata fields (must be literals).
    std::map<cstring, const IR::Literal *> metadataFields;

 public:
    MetadataCollection();

    [[nodiscard]] cstring getObjectName() const override;

    [[nodiscard]] const MetadataCollection *evaluate(const Model & /*model*/,
                                                     bool doComplete) const override;

    /// @returns the list of metadata fields.
    [[nodiscard]] const std::map<cstring, const IR::Literal *> &getMetadataFields() const;

    /// Add a metadata field to the collection.
    void addMetaDataField(cstring name, const IR::Literal *metadataField);

    /// @returns a metadata field from the collection.
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

    [[nodiscard]] const Optional *evaluate(const Model &model, bool doComplete) const override;

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the match value. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    [[nodiscard]] const IR::Constant *getEvaluatedValue() const;

    /// @returns whether to add this optional match as an exact match.
    [[nodiscard]] bool addAsExactMatch() const;

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

    [[nodiscard]] const Range *evaluate(const Model &model, bool doComplete) const override;

    [[nodiscard]] cstring getObjectName() const override;

    /// @returns the inclusive start of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    [[nodiscard]] const IR::Constant *getEvaluatedLow() const;

    /// @returns the inclusive end of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    [[nodiscard]] const IR::Constant *getEvaluatedHigh() const;

    DECLARE_TYPEINFO(Range, TableMatch);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_SPEC_H_ */
