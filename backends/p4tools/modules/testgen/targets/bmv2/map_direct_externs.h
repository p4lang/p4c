#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_

#include <map>

#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// A lightweight visitor, which collects all the declarations in the program then checks whether a
/// table is referencing the declaration as an direct extern. The direct externs are collected in a
/// map. Currently checks for "counters" or "meters" properties in the table.
class MapDirectExterns : public Inspector {
 private:
    /// List of the declared instances in the program.
    std::map<cstring, const IR::Declaration_Instance *> declaredExterns;

    /// Maps direct extern declarations to the table they are attached to.
    std::map<const IR::IDeclaration *, const IR::P4Table *> directExternMap;

 public:
    bool preorder(const IR::Declaration_Instance *declInstance) override;

    bool preorder(const IR::P4Table *table) override;

    /// @returns the list of direct externs and their corresponding table.
    const std::map<const IR::IDeclaration *, const IR::P4Table *> &getdirectExternMap();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_*/
