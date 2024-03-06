#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_EXPRESSIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_EXPRESSIONS_H_

#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/ir.h"

namespace P4Tools::P4Smith {

using TyperefProbs = struct TyperefProbs {
    int64_t p4_bit;
    int64_t p4_signed_bit;
    int64_t p4_varbit;
    int64_t p4_int;
    int64_t p4_error;
    int64_t p4_bool;
    int64_t p4_string;
    // derived types
    int64_t p4_enum;
    int64_t p4_header;
    int64_t p4_header_stack;
    int64_t p4_struct;
    int64_t p4_header_union;
    int64_t p4_tuple;
    int64_t p4_void;
    int64_t p4_match_kind;
};

class Expressions {
    static constexpr size_t MAX_DEPTH = 3;

    static constexpr int bit_widths[6] = {4, 8, 16, 32, 64, 128};

 public:
    static IR::Type_Boolean *genBoolType();

    static IR::Type_InfInt *genIntType();

    // isSigned, true -> int<>, false -> bit<>
    static IR::Type_Bits *genBitType(bool isSigned);

    static IR::Type *pickRndBaseType(const std::vector<int64_t> &type_probs);

    virtual IR::Type *pickRndType(TyperefProbs type_probs);

    IR::BoolLiteral *genBoolLiteral();

    IR::Constant *genIntLiteral(size_t bit_width = INTEGER_WIDTH);

    IR::Constant *genBitLiteral(const IR::Type *tb);

 private:
    IR::Expression *constructUnaryExpr(const IR::Type_Bits *tb);

    IR::Expression *createSaturationOperand(const IR::Type_Bits *tb);

    IR::Expression *constructBinaryBitExpr(const IR::Type_Bits *tb);

    IR::Expression *constructTernaryBitExpr(const IR::Type_Bits *tb);

 public:
    virtual IR::Expression *pickBitVar(const IR::Type_Bits *tb);

    virtual IR::Expression *constructBitExpr(const IR::Type_Bits *tb);

 private:
    IR::Expression *constructCmpExpr();

 public:
    virtual IR::Expression *constructBooleanExpr();

 private:
    IR::Expression *constructUnaryIntExpr();

    IR::Expression *constructBinaryIntExpr();

    IR::Expression *pickIntVar();

 public:
    IR::Expression *constructIntExpr();

 private:
    IR::ListExpression *genStructListExpr(const IR::Type_Name *tn);

 public:
    virtual IR::Expression *constructStructExpr(const IR::Type_Name *tn);

    virtual IR::MethodCallExpression *genFunctionCall(cstring method_name,
                                                      IR::ParameterList params);

    virtual IR::MethodCallExpression *pickFunction(
        IR::IndexedVector<IR::Declaration> viable_functions, const IR::Type **ret_type);

    virtual IR::Expression *genExpression(const IR::Type *tp);

    virtual IR::ListExpression *genExpressionList(IR::Vector<IR::Type> types, bool only_lval);

    virtual IR::Expression *genInputArg(const IR::Parameter *param);

    virtual bool checkInputArg(const IR::Parameter *param);
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_EXPRESSIONS_H_ */
