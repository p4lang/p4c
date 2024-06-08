#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_TARGET_H_

#include <ostream>
#include <string>

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/ir.h"

namespace P4Tools::P4Smith::Generic {

class AbstractGenericSmithTarget : public SmithTarget {
 protected:
    explicit AbstractGenericSmithTarget(const std::string &deviceName, const std::string &archName);
};

class GenericCoreSmithTarget : public AbstractGenericSmithTarget {
 private:
    DeclarationGenerator *_declarationGenerator = new DeclarationGenerator(*this);
    ExpressionGenerator *_expressionGenerator = new ExpressionGenerator(*this);
    StatementGenerator *_statementGenerator = new StatementGenerator(*this);
    ParserGenerator *_parserGenerator = new ParserGenerator(*this);
    TableGenerator *_tableGenerator = new TableGenerator(*this);

    [[nodiscard]] IR::P4Parser *generateParserBlock() const;
    [[nodiscard]] IR::P4Control *generateIngressBlock() const;
    [[nodiscard]] static IR::Declaration_Instance *generateMainPackage();

    [[nodiscard]] IR::Type_Parser *generateParserBlockType() const;
    [[nodiscard]] IR::Type_Control *generateIngressBlockType() const;
    [[nodiscard]] IR::Type_Package *generatePackageType() const;

 public:
    /// Registers this target.
    static void make();

    [[nodiscard]] int writeTargetPreamble(std::ostream *ostream) const override;

    [[nodiscard]] const IR::P4Program *generateP4Program() const override;

    [[nodiscard]] DeclarationGenerator &declarationGenerator() const override {
        return *_declarationGenerator;
    }

    [[nodiscard]] ExpressionGenerator &expressionGenerator() const override {
        return *_expressionGenerator;
    }

    [[nodiscard]] StatementGenerator &statementGenerator() const override {
        return *_statementGenerator;
    }

    [[nodiscard]] ParserGenerator &parserGenerator() const override { return *_parserGenerator; }

    [[nodiscard]] TableGenerator &tableGenerator() const override { return *_tableGenerator; }

 private:
    GenericCoreSmithTarget();
};

}  // namespace P4Tools::P4Smith::Generic

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_GENERIC_TARGET_H_ */
