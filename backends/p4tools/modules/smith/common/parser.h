#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_PARSER_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_PARSER_H_

#include "backends/p4tools/modules/smith/common/generator.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"

namespace P4Tools::P4Smith {

class ParserGenerator : public Generator {
 private:
    IR::IndexedVector<IR::ParserState> state_list;

 public:
    virtual ~ParserGenerator() = default;
    explicit ParserGenerator(const SmithTarget &target) : Generator(target) {}

    virtual IR::MethodCallStatement *genHdrExtract(IR::Member *pkt_call, IR::Expression *mem);
    virtual void genHdrUnionExtract(IR::IndexedVector<IR::StatOrDecl> &components,
                                    const IR::Type_HeaderUnion *hdru, IR::ArrayIndex *arr_ind,
                                    IR::Member *pkt_call);
    virtual IR::ListExpression *buildMatchExpr(IR::Vector<IR::Type> types);
    virtual IR::ParserState *genStartState();
    virtual IR::ParserState *genHdrStates();
    virtual void genState(cstring name);
    virtual void buildParserTree();

    [[nodiscard]] IR::IndexedVector<IR::ParserState> getStates() const { return state_list; }
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_PARSER_H_ */
