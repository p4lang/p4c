#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_MIDEND_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_MIDEND_H_

#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "midend/convertEnums.h"
#include "midend/convertErrors.h"

namespace P4Tools {

/// Implements a generic mid end for all targets. If needed, targets can customize this to
/// implement things like targets-specific conversion of P4 enums to bit<n>.
///
/// Mid-end implementations must establish certain invariants in the IR.
///
/// TODO: Document these invariants. So far, we have:
///   * controls and parsers are inlined
///   * function arguments are propagated into function bodies where possible
///   * fully type-checked
class MidEnd : public PassManager {
 protected:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    /// Provides a target-specific pass that converts P4 enums to bit<n>. The default
    /// implementation returns P4::ConvertEnums, instantiated with the policy provided by
    /// @mkChooseEnumRepresentation.
    virtual Visitor *mkConvertEnums();

    /// Provides a target-specific pass that converts P4 errors to bit<n>. The default
    /// implementation returns P4Tools::ConvertErrors, instantiated with the policy provided by
    /// @mkChooseEnumRepresentation.
    virtual Visitor *mkConvertErrors();

    /// Provides a target-specific pass that simplifies keys in table calls under a custom policy.
    virtual Visitor *mkConvertKeys();

    /// Provides a target-specific policy for converting P4 enums to bit<n>. The default
    /// implementation converts all enums to bit<32>.
    virtual P4::ChooseEnumRepresentation *mkConvertEnumsPolicy();

    /// Provides a target-specific policy for converting P4 error to bit<n>. The default
    /// implementation converts all errors to bit<32>.
    virtual P4::ChooseErrorRepresentation *mkConvertErrorPolicy();

    /// Provides a target-specific policy for determining when to do local copy propagation.
    /// Implementations should return @false if local copy propagation should not be performed on
    /// the given expression. The default implementation enables local copy propagation everywhere
    /// by always returning @true.
    virtual bool localCopyPropPolicy(const Visitor::Context *ctx, const IR::Expression *expr);

 public:
    explicit MidEnd(const CompilerOptions &);

    /// Retrieve the reference map used in the mid end.
    P4::ReferenceMap *getRefMap();

    /// Retrieve the type map used in the mid end.
    P4::TypeMap *getTypeMap();

    /// Add the list of default passes to the mid end. This is not part of the initializer because
    /// some targets may add their own passes to the beginning of the pass list.
    void addDefaultPasses();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_MIDEND_H_ */
