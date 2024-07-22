#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_V1MODEL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_V1MODEL_H_

#include <ostream>

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/targets/bmv2/target.h"
#include "ir/ir.h"

namespace P4C::P4Tools::P4Smith::BMv2 {

class Bmv2V1modelSmithTarget : public AbstractBMv2SmithTarget {
 private:
    DeclarationGenerator *_declarationGenerator = new DeclarationGenerator(*this);
    ExpressionGenerator *_expressionGenerator = new ExpressionGenerator(*this);
    StatementGenerator *_statementGenerator = new StatementGenerator(*this);
    ParserGenerator *_parserGenerator = new ParserGenerator(*this);
    TableGenerator *_tableGenerator = new TableGenerator(*this);

    [[nodiscard]] IR::P4Parser *generateParserBlock() const;
    [[nodiscard]] IR::P4Control *generateIngressBlock() const;
    [[nodiscard]] IR::P4Control *generateUpdateBlock() const;
    [[nodiscard]] IR::P4Control *generateVerifyBlock() const;
    [[nodiscard]] IR::P4Control *generateEgressBlock() const;
    [[nodiscard]] IR::P4Control *generateDeparserBlock() const;

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
    Bmv2V1modelSmithTarget();
};

}  // namespace P4C::P4Tools::P4Smith::BMv2

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_V1MODEL_H_ */
