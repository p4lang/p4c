#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_

#include <ostream>
#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "ir/ir.h"

namespace p4c::P4Tools::P4Smith {

class SmithTarget : public CompilerTarget {
 public:
    /// @returns the singleton instance for the current target.
    static const SmithTarget &get();

    /// Write the necessary #include directives and other helpful constructs to the specified
    /// stream.
    [[nodiscard]] virtual int writeTargetPreamble(std::ostream *ostream) const = 0;

    [[nodiscard]] virtual const IR::P4Program *generateP4Program() const = 0;

    [[nodiscard]] virtual DeclarationGenerator &declarationGenerator() const = 0;
    [[nodiscard]] virtual ExpressionGenerator &expressionGenerator() const = 0;
    [[nodiscard]] virtual StatementGenerator &statementGenerator() const = 0;
    [[nodiscard]] virtual ParserGenerator &parserGenerator() const = 0;
    [[nodiscard]] virtual TableGenerator &tableGenerator() const = 0;

 protected:
    explicit SmithTarget(const std::string &deviceName, const std::string &archName);

 private:
};

}  // namespace p4c::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_ */
