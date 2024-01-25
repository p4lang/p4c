#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4RUNTIME_TRANSLATION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4RUNTIME_TRANSLATION_H_
#include <functional>

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Propagates P4Runtime annotations attached to type definitions to the nodes which use these type
/// definitions. For now, this is restricted to key elements and action parameters.
class PropagateP4RuntimeTranslation : public Transform {
    /// We use the typemap to look up the original declaration for type reference.
    /// These declarations may have an annotation.
    std::reference_wrapper<const P4::TypeMap> _typeMap;

    /// Look up annotations relevant to P4Runtime. They may influence the control plane interfaces.
    static std::vector<const IR::Annotation *> lookupP4RuntimeAnnotations(
        const P4::TypeMap &typeMap, const IR::Type *type);

    const IR::Parameter *preorder(IR::Parameter *parameter) override;
    const IR::KeyElement *preorder(IR::KeyElement *keyElement) override;

 public:
    explicit PropagateP4RuntimeTranslation(const P4::TypeMap &typeMap);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4RUNTIME_TRANSLATION_H_ */
