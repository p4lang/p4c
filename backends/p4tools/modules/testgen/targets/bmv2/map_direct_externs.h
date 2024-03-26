#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_

#include <array>
#include <map>
#include <optional>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// A mapping of the control plane name of extern declarations which are associated with a table.
/// Such an extern is referred to as direct extern. There can only be one extern associated with a
/// table in BMv2.
/// We are using the cstring name and not an IR::Declaration pointer for the mapping because other
/// passes may clone or transform the IR::Declaration node, invalidating this mapping.
using DirectExternMap = std::map<cstring, const IR::P4Table *>;

/// A lightweight visitor, which collects all the declarations in the program then checks whether a
/// table is referencing the declaration as an direct extern. The direct externs are collected in a
/// map. Currently checks for "counters" or "meters" properties in the table.
class MapDirectExterns : public Inspector {
 private:
    /// The table property labels which are associated with an extern declaration.
    static constexpr std::array kTableExternProperties = {"meters", "counters"};

    /// List of the declared instances in the program.
    std::map<cstring, const IR::Declaration_Instance *> declaredExterns;

    /// Maps direct extern declarations to the table they are attached to.
    DirectExternMap directExternMap;

    /// Tries to lookup the associated extern declaration from a table implementation property.
    /// @returns std::nullopt if the declaration can not be found.
    std::optional<const IR::Declaration_Instance *> getExternFromTableImplementation(
        const IR::Property *tableImplementation);

    bool preorder(const IR::Declaration_Instance *declInstance) override;
    bool preorder(const IR::P4Table *table) override;

 public:
    /// @returns the list of direct externs and their corresponding table.
    const DirectExternMap &getDirectExternMap();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_MAP_DIRECT_EXTERNS_H_*/
