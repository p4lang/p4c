#include "backends/p4tools/modules/smith/targets/bmv2/declarations.h"

#include <cstdlib>
#include <string>

namespace P4Tools::P4Smith {

IR::ParameterList *Bmv2V1ModelDeclarationGenerator::genParameterList() {
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

}  // namespace P4Tools::P4Smith
