#ifndef BACKENDS_P4TOOLS_COMMON_LIB_TABLE_UTILS_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_TABLE_UTILS_H_

#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools::TableUtils {

/// KeyProperties define properties of table keys that are useful for execution.
struct KeyProperties {
    /// The original key element
    IR::KeyElement const *key;

    /// The control plane name of the key.
    cstring name;

    /// The index of this key within the current table.
    size_t index;

    /// The match type of this key (exact, ternary, lpm, etc...)
    cstring matchType;

    /// Whether the key is tainted.
    /// For matches such as LPM and ternary, we can still generate a match.
    bool isTainted;

    explicit KeyProperties(IR::KeyElement const *key, cstring name, size_t index, cstring matchType,
                           bool isTainted)
        : key(key), name(name), index(index), matchType(matchType), isTainted(isTainted) {}
};

/// Basic table properties that are set when initializing the TableStepper.
struct TableProperties {
    /// Control plane name of the table.
    cstring tableName;

    /// Whether key expressions of the table contain taint. This makes it difficult to match.
    bool tableIsTainted = false;

    /// Whether the table is constant and can not accept control plane entries.
    bool tableIsImmutable = false;

    /// Indicates whether the default action can be overridden.
    bool defaultIsImmutable = false;

    /// Ordered list of key fields with useful properties.
    std::vector<KeyProperties> resolvedKeys;

    /// Maps an action in the action list to a numerical identifier.
    /// We do not use IR::MethodCallExpression here because we also look up switch labels.
    std::map<cstring, int> actionIdMap;
};

/// Checks whether certain table properties are immutable and sets the properties param
/// accordingly.
void checkTableImmutability(const IR::P4Table &table, TableProperties &properties);

/// Collects the list of actions contained in the table and filters out @defaultonly actions.
std::vector<const IR::ActionListElement *> buildTableActionList(const IR::P4Table &table);

/// The function compares the right and left expression.
/// Returns true if the values are equal or if the left value is more precise.
/// If the left value is constant entries and the right value
/// is Mask/Default then priority is given to constants.
/// For more detailed information see
/// https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md#longest-prefix-match-tables
/// @param lpmIndex indicates the index of the lpm entry in the list of entries.
/// It is used for comparison.
bool compareLPMEntries(const IR::Entry *leftIn, const IR::Entry *rightIn, size_t lpmIndex);

/// @returns the name of the default action of the table as path expression.
const IR::PathExpression *getDefaultActionName(const IR::P4Table &table);

/// Matches the match values of an entry with the values and variables of the key.
/// @returns a boolean expression describing the match.
/// Returns a false IR::BoolLiteral if there are no keys.
const IR::Expression *computeEntryMatch(const IR::P4Table &table, const IR::Entry &entry,
                                        const IR::Key &key);

}  // namespace P4Tools::TableUtils

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_TABLE_UTILS_H_ */
