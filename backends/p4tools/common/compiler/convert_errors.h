#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_ERRORS_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_ERRORS_H_

#include <map>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/safe_vector.h"
#include "midend/convertEnums.h"

namespace P4Tools {

/**
 * Policy function: given a number of error values should return
 * the size of a Type_Bits type used to represent the values.
 * This class is lefted from enum conversion path.
 */
class ChooseErrorRepresentation {
 public:
    virtual ~ChooseErrorRepresentation() {}
    // If true this type has to be converted.
    virtual bool convert(const IR::Type_Error* type) const = 0;
    // errorCount is the number of different error values.
    // The returned value is the width of Type_Bits used
    // to represent the error.  Obviously, we must have
    // 2^(return) >= errorCount.
    virtual unsigned errorSize(unsigned errorCount) const = 0;
};

class DoConvertErrors : public Transform {
    friend class ConvertErrors;

    std::map<cstring, P4::EnumRepresentation*> repr;
    ChooseErrorRepresentation* policy;
    P4::TypeMap* typeMap;

 public:
    DoConvertErrors(ChooseErrorRepresentation* policy, P4::TypeMap* typeMap)
        : policy(policy), typeMap(typeMap) {
        CHECK_NULL(policy);
        CHECK_NULL(typeMap);
        setName("DoConvertErrors");
    }
    const IR::Node* preorder(IR::Type_Error* type) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
    const IR::Node* postorder(IR::Member* member) override;
};

class ConvertErrors : public PassManager {
    DoConvertErrors* convertErrors{nullptr};

 public:
    using ErrorMapping = decltype(DoConvertErrors::repr);
    ConvertErrors(P4::ReferenceMap* refMap, P4::TypeMap* typeMap, ChooseErrorRepresentation* policy,
                  P4::TypeChecking* typeChecking = nullptr)
        : convertErrors(new DoConvertErrors(policy, typeMap)) {
        if (!typeChecking) typeChecking = new P4::TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(convertErrors);
        passes.push_back(new P4::ClearTypeMap(typeMap));
        setName("ConvertErrors");
    }

    ErrorMapping getErrorMapping() const { return convertErrors->repr; }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_ERRORS_H_ */
