#include "backends/p4tools/modules/testgen/targets/bmv2/concolic.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/add.hpp>
#include <boost/multiprecision/cpp_int/import_export.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/model.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/nethash.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"

namespace P4Tools::P4Testgen::Bmv2 {

static big_int checksum(Bmv2HashAlgorithm algo, const uint8_t *buf, size_t len) {
    // Pick a checksum according to the algorithm value.
    switch (algo) {
        case Bmv2HashAlgorithm::csum16:
            return NetHash::csum16(buf, len);
        case Bmv2HashAlgorithm::crc32:
            return NetHash::crc32(buf, len);
        case Bmv2HashAlgorithm::crc16:
            return NetHash::crc16(buf, len);
        case Bmv2HashAlgorithm::identity:
            return NetHash::identity(buf, len);
        case Bmv2HashAlgorithm::xor16:
            return NetHash::xor16(buf, len);
        case Bmv2HashAlgorithm::random: {
            BUG("Random should not be encountered here");
        }
        case Bmv2HashAlgorithm::crc32_custom:
        case Bmv2HashAlgorithm::crc16_custom:
        default:
            TESTGEN_UNIMPLEMENTED("Algorithm %1% not implemented for hash.", algo);
    }
}

big_int Bmv2Concolic::computeChecksum(const std::vector<const IR::Expression *> &exprList,
                                      const Model &finalModel, Bmv2HashAlgorithm algo,
                                      Model::ExpressionMap *resolvedExpressions) {
    std::vector<uint8_t> bytes;
    if (!exprList.empty()) {
        const auto *concatExpr = exprList.at(0);
        for (size_t idx = 1; idx < exprList.size(); idx++) {
            const auto *expr = exprList.at(idx);
            const auto *newWidth =
                IR::getBitType(concatExpr->type->width_bits() + expr->type->width_bits());
            concatExpr = new IR::Concat(newWidth, concatExpr, expr);
        }

        auto concatWidth = concatExpr->type->width_bits();
        auto remainder = concatExpr->type->width_bits() % CHUNK_SIZE;
        // The behavioral model pads widths less than CHUNK_SIZE from the right side.
        // If we have a remainder, add another concatenation here.
        if (remainder != 0) {
            auto fillWidth = CHUNK_SIZE - remainder;
            concatWidth += fillWidth;
            const auto *remainderExpr = IR::getConstant(IR::getBitType(fillWidth), 0);
            concatExpr = new IR::Concat(IR::getBitType(concatWidth), concatExpr, remainderExpr);
        }
        auto dataInt =
            IR::getBigIntFromLiteral(finalModel.evaluate(concatExpr, true, resolvedExpressions));
        bytes = convertBigIntToBytes(dataInt, concatWidth, true);
    }
    return checksum(algo, bytes.data(), bytes.size());
}

const ConcolicMethodImpls::ImplList Bmv2Concolic::BMV2_CONCOLIC_METHOD_IMPLS{
    /* ======================================================================================
     *   method_hash
     * ====================================================================================== */
    /// Calculate a hash function of the value specified by the data
    /// parameter.  The value written to the out parameter named result
    /// will always be in the range [base, base+max-1] inclusive, if max >=
    /// 1.  If max=0, the value written to result will always be base.
    /// Note that the types of all of the parameters may be the same as, or
    /// different from, each other, and thus their bit widths are allowed
    /// to be different.
    /// @param O          Must be a type bit<W>
    /// @param D          Must be a tuple type where all the fields are bit-fields (type bit<W> or
    /// int<W>) or varbits.
    /// @param T          Must be a type bit<W>
    /// @param M          Must be a type bit<W>
    {"*method_hash",
     {"result", "algo", "base", "data", "max"},
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable *var,
        const ExecutionState & /*state*/, const Model &finalModel,
        ConcolicVariableMap *resolvedConcolicVariables) {
         const auto *args = var->arguments;
         const auto *checksumVar = args->at(0)->expression;
         if (!(checksumVar->is<IR::Member>() || checksumVar->is<IR::PathExpression>())) {
             TESTGEN_UNIMPLEMENTED("Checksum input %1% of type %2% not supported", checksumVar,
                                   checksumVar->node_type_name());
         }
         // Assign arguments to concrete variables and perform type checking.
         auto algo = Bmv2HashAlgorithm(args->at(1)->expression->checkedTo<IR::Constant>()->asInt());
         Model::ExpressionMap resolvedExpressions;
         const auto *base =
             finalModel.evaluate(args->at(2)->expression, true, &resolvedExpressions);
         auto baseInt = IR::getBigIntFromLiteral(base);
         const auto *dataExpr = args->at(3)->expression;
         const auto *maxHash =
             finalModel.evaluate(args->at(4)->expression, true, &resolvedExpressions);
         auto maxHashInt = IR::getBigIntFromLiteral(maxHash);

         /// Flatten the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto *dataList = dataExpr->checkedTo<IR::StructExpression>();
         const auto exprList = IR::flattenStructExpression(dataList);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         if (maxHashInt == 0) {
             computedResult = baseInt;
         } else {
             computedResult = computeChecksum(exprList, finalModel, algo, &resolvedExpressions);
             // Behavioral model uses this technique to limit the hash output.
             computedResult = (baseInt + (computedResult % maxHashInt));
         }

         // Assign a value to the @param result using the computed result
         if (const auto *checksumVarType = checksumVar->type->to<IR::Type_Bits>()) {
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[*var] = IR::getConstant(checksumVarType, computedResult);

         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVar->type);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         // We can not use resolvedConcolic variables here because there might be multiple resolved
         // expressions for a single concolic variable.
         for (const auto &variable : resolvedExpressions) {
             const auto *varName = variable.first;
             const auto *varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},
    /* ======================================================================================
     *   method_checksum
     * ====================================================================================== */
    /// This method is almost equivalent to the hash method. Except that when the checksum output is
    /// out of bounds, this method assigns the maximum instead of using a modulo operation.
    {"*method_checksum",
     {"result", "algo", "data"},
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable *var,
        const ExecutionState & /*state*/, const Model &finalModel,
        ConcolicVariableMap *resolvedConcolicVariables) {
         // Assign arguments to concrete variables and perform type checking.
         const auto *args = var->arguments;
         const auto *checksumVar = args->at(0)->expression;
         auto algo = Bmv2HashAlgorithm(args->at(1)->expression->checkedTo<IR::Constant>()->asInt());
         const auto *dataExpr = args->at(2)->expression;
         const auto *checksumVarType = checksumVar->type;
         // This is the maximum value this checksum can have.
         auto maxHashInt = IR::getMaxBvVal(checksumVarType);

         /// Iterate through the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto *dataList = dataExpr->checkedTo<IR::StructExpression>();
         const std::vector<const IR::Expression *> exprList = IR::flattenStructExpression(dataList);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         Model::ExpressionMap resolvedExpressions;
         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         computedResult = computeChecksum(exprList, finalModel, algo, &resolvedExpressions);
         // Behavioral model uses this technique to limit the checksum output.
         computedResult = std::min(computedResult, maxHashInt);
         // Assign a value to the @param result using the computed result
         if (checksumVarType->is<IR::Type_Bits>()) {
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[*var] = IR::getConstant(checksumVarType, computedResult);
         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVarType);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         // We can not use resolvedConcolic variables here because there might be multiple resolved
         // expressions for a single concolic variable.
         for (const auto &variable : resolvedExpressions) {
             const auto *varName = variable.first;
             const auto *varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},

    {"*method_checksum_with_payload",
     {"result", "algo", "data"},
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable *var,
        const ExecutionState & /*state*/, const Model &finalModel,
        ConcolicVariableMap *resolvedConcolicVariables) {
         // Assign arguments to concrete variables and perform type checking.
         const auto *args = var->arguments;
         const auto *checksumVar = args->at(0)->expression;
         auto algo = Bmv2HashAlgorithm(args->at(1)->expression->checkedTo<IR::Constant>()->asInt());
         const auto *dataExpr = args->at(2)->expression;
         const auto *checksumVarType = checksumVar->type;
         Model::ExpressionMap resolvedExpressions;

         /// Iterate through the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto *data = dataExpr->checkedTo<IR::StructExpression>();
         auto exprList = IR::flattenStructExpression(data);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         // This is the maximum value this checksum can have.
         auto maxHashInt = IR::getMaxBvVal(checksumVarType);
         // If the payload is present, we need to add it to our checksum calculation.
         const auto *payloadExpr = finalModel.get(&PacketVars::PAYLOAD_SYMBOL, false);
         if (payloadExpr != nullptr) {
             exprList.push_back(payloadExpr);
         }

         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         computedResult = computeChecksum(exprList, finalModel, algo, &resolvedExpressions);
         // Behavioral model uses this technique to limit the checksum output.
         computedResult = std::min(computedResult, maxHashInt);
         // Assign a value to the @param result using the computed result
         if (checksumVarType->is<IR::Type_Bits>()) {
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[*var] = IR::getConstant(checksumVarType, computedResult);
         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVarType);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         // We can not use resolvedConcolic variables here because there might be multiple resolved
         // expressions for a single concolic variable.
         for (const auto &variable : resolvedExpressions) {
             const auto *varName = variable.first;
             const auto *varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},
};

const ConcolicMethodImpls::ImplList *Bmv2Concolic::getBmv2ConcolicMethodImpls() {
    return &BMV2_CONCOLIC_METHOD_IMPLS;
}

std::ostream &operator<<(std::ostream &os, Bmv2HashAlgorithm algo) {
#define ALGO_CASE(A)           \
    case Bmv2HashAlgorithm::A: \
        return os << #A << " [" << int(algo) << "]"
    switch (algo) {
        ALGO_CASE(crc32);
        ALGO_CASE(crc32_custom);
        ALGO_CASE(crc16);
        ALGO_CASE(crc16_custom);
        ALGO_CASE(random);
        ALGO_CASE(identity);
        ALGO_CASE(csum16);
        ALGO_CASE(xor16);
        // No default: let the compiler produce a warning if some defined enum value is not covered.
    }
#undef ALGO_CASE
    // For values other then the declared ones.
    return os << "INVALID [" << int(algo) << "]";
}

}  // namespace P4Tools::P4Testgen::Bmv2
