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

namespace P4Tools::P4Smith::BMv2 {

class BMv2DeclarationGenerator;
class Bmv2V1modelSmithTarget;

class BMv2DeclarationGenerator : public P4Tools::P4Smith::DeclarationGenerator {
 private:
    Bmv2V1modelSmithTarget &_target;

 public:
    // TODO(zzmic): Figure out how to make this contructor right.
    // Two concerns:
    // 1. There exists a circular dependency between BMv2DeclarationGenerator and Bmv2V1modelSmithTarget.
    // 2. The inheritance chain is unclear.
    // In essenece, all I want is to have a custom genParameterList method for the BMv2 target.
    explicit BMv2DeclarationGenerator(Bmv2V1modelSmithTarget &target)
        : DeclarationGenerator(static_cast<SmithTarget &>(target)), _target(target) {}

    IR::ParameterList *genParameterList() override;
};

class Bmv2V1modelSmithTarget : public AbstractBMv2SmithTarget {
 private:
    DeclarationGenerator *_declarationGenerator;
    ExpressionGenerator *_expressionGenerator;
    StatementGenerator *_statementGenerator;
    ParserGenerator *_parserGenerator;
    TableGenerator *_tableGenerator;

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

    friend class BMv2DeclarationGenerator;

 private:
    Bmv2V1modelSmithTarget();
};

}  // namespace P4Tools::P4Smith::BMv2

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_V1MODEL_H_ */
