#ifndef FRONTENDS_P4_GETV1MODELVERSION_H_
#define FRONTENDS_P4_GETV1MODELVERSION_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4::P4V1 {

/// Stores the version of the global constant __v1model_version used
/// in the 'version' public instance variable.
class GetV1ModelVersion : public Inspector {
    bool preorder(const IR::Declaration_Constant *dc) override {
        if (dc->name == "__v1model_version") {
            const auto *val = dc->initializer->to<IR::Constant>();
            if (!val || val->value < 0) {
                P4::error(ErrorType::ERR_EXPECTED, "%1%: expected v1model version to be a non-negative number", dc->initializer);
                return false;
            }
            version = static_cast<unsigned>(val->value);
        }
        return false;
    }
    bool preorder(const IR::Declaration *) override { return false; }

 public:
    unsigned version = 0;
};

}  // namespace P4::P4V1

#endif /* FRONTENDS_P4_GETV1MODELVERSION_H_ */
