#ifndef FRONTENDS_P4_GETV1MODELVERSION_H_
#define FRONTENDS_P4_GETV1MODELVERSION_H_

#include "absl/strings/numbers.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
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
                P4::error(ErrorType::ERR_EXPECTED,
                          "%1%: expected v1model version to be a non-negative number",
                          dc->initializer);
                return false;
            }
            version = static_cast<unsigned>(val->value);

            unsigned initialVersion;
            unsigned currentVersion;
            BUG_CHECK(absl::SimpleAtoi(V1Model::versionInitial, &initialVersion) &&
                          absl::SimpleAtoi(V1Model::versionCurrent, &currentVersion),
                      "Unable to check v1model version");
            if (version < initialVersion || version > currentVersion) {
                P4::error(
                    ErrorType::ERR_UNKNOWN,
                    "%1%: unknown v1model version; initial supported version is %2%, latest is %3%",
                    dc->initializer, initialVersion, currentVersion);
                return false;
            }
        }
        return false;
    }
    bool preorder(const IR::Declaration *) override { return false; }

 public:
    unsigned version = 0;
};

}  // namespace P4::P4V1

#endif /* FRONTENDS_P4_GETV1MODELVERSION_H_ */
