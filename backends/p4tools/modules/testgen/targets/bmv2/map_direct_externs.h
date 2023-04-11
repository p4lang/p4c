#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_

#include <map>

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::P4Testgen::Bmv2 {

class MapDirectExterns : public Inspector {
 private:
    std::map<const IR::IDeclaration *, const IR::P4Table *> directExternMap;

    std::map<cstring, const IR::Declaration_Instance *> declaredExterns;

 public:
    MapDirectExterns();

    bool preorder(const IR::Declaration_Instance *declInstance) override;

    bool preorder(const IR::P4Table *table) override;

    std::map<const IR::IDeclaration *, const IR::P4Table *> getdirectExternMap();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_*/
