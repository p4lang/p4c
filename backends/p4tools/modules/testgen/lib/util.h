#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_UTIL_H_

#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools::P4Testgen {

/// Converts the list of arguments @inputArgs to a list of type declarations. Any names appearing in
/// the arguments are resolved with @ns.
/// This is mainly useful for inspecting package instances.
std::vector<const IR::Type_Declaration *> argumentsToTypeDeclarations(
    const IR::IGeneralNamespace *ns, const IR::Vector<IR::Argument> *inputArgs);

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_UTIL_H_ */
