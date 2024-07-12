#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_

#include <string>

#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4Tools::P4Smith::BMv2 {

class AbstractBMv2SmithTarget : public SmithTarget {
 protected:
    explicit AbstractBMv2SmithTarget(const std::string &deviceName, const std::string &archName);
};
}  // namespace P4Tools::P4Smith::BMv2

namespace P4Tools::P4Smith {

using namespace P4::literals;

class Bmv2V1ModelDeclarationGenerator : public DeclarationGenerator {
 public:
    explicit Bmv2V1ModelDeclarationGenerator(
        P4Tools::P4Smith::BMv2::AbstractBMv2SmithTarget &parent)
        : DeclarationGenerator(static_cast<P4Tools::P4Smith::SmithTarget &>(parent)) {}

    IR::ParameterList *genParameterList() override {
        IR::IndexedVector<IR::Parameter> params;
        size_t totalParams = Utils::getRandInt(0, 3);
        size_t numDirParams = (totalParams != 0U) ? Utils::getRandInt(0, totalParams - 1) : 0;
        size_t numDirectionlessParams = totalParams - numDirParams;
        for (size_t i = 0; i < numDirParams; i++) {
            TyperefProbs typePercent = {
                PCT.PARAMETER_NONEDIR_BASETYPE_BIT,
                PCT.PARAMETER_NONEDIR_BASETYPE_SIGNED_BIT,
                0,
                PCT.PARAMETER_NONEDIR_BASETYPE_INT,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            };
            IR::Parameter *param = this->genTypedParameter(false, typePercent);
            if (param == nullptr) {
                BUG("param is null");
            }
            params.push_back(param);
            // add to the scope
            P4Scope::addToScope(param);
            // only add values that are not read-only to the modifiable types
            if (param->direction == IR::Direction::In) {
                P4Scope::addLval(param->type, param->name.name, true);
            } else {
                P4Scope::addLval(param->type, param->name.name, false);
            }
        }
        for (size_t i = 0; i < numDirectionlessParams; i++) {
            TyperefProbs typePercent = {
                PCT.PARAMETER_BASETYPE_BIT,
                PCT.PARAMETER_BASETYPE_SIGNED_BIT,
                0,
                PCT.PARAMETER_BASETYPE_INT,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            };
            IR::Parameter *param = this->genTypedParameter(true, typePercent);

            if (param == nullptr) {
                BUG("param is null");
            }
            params.push_back(param);
            // add to the scope
            P4Scope::addToScope(param);
            P4Scope::addLval(param->type, param->name.name, true);
        }

        return new IR::ParameterList(params);
    }
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_ */
