#include "backends/p4tools/testgen/targets/bmv2/concolic.h"

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

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen/lib/exceptions.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/targets/bmv2/contrib/bmv2_hash/calculations.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

std::vector<char> Bmv2Concolic::convertBigIntToBytes(big_int& dataInt, int targetWidthBits) {
    std::vector<char> bytes;
    // Convert the input bit width to bytes and round up.
    size_t targetWidthBytes = (targetWidthBits + CHUNK_SIZE - 1) / CHUNK_SIZE;
    boost::multiprecision::export_bits(dataInt, std::back_inserter(bytes), CHUNK_SIZE);
    // If the number of bytes produced by the export is lower than the desired width pad the byte
    // array with zeroes.
    auto diff = targetWidthBytes - bytes.size();
    if (targetWidthBytes > bytes.size() && diff > 0UL) {
        for (size_t i = 0; i < diff; ++i) {
            bytes.insert(bytes.begin(), 0);
        }
    }

    return bytes;
}

const IR::Expression* Bmv2Concolic::setAndComputePayload(
    const Model& completedModel, ConcolicVariableMap* resolvedConcolicVariables, int payloadSize) {
    const auto* payloadType = IRUtils::getBitType(payloadSize);
    const auto& payLoadVar = StateVariable(ExecutionState::getPayloadLabel(payloadType));
    const auto* payloadExpr = completedModel.get(payLoadVar, false);
    // If the variable already has been fixed, return it
    auto it = resolvedConcolicVariables->find(payLoadVar);
    if (it != resolvedConcolicVariables->end()) {
        return it->second;
    }
    payloadExpr = IRUtils::getRandConstantForType(payloadType);
    BUG_CHECK(payloadExpr->type->width_bits() == payloadSize,
              "The width (%1%) of the payload expression should match the calculated payload "
              "size %2%.",
              payloadExpr->type->width_bits(), payloadSize);
    // Set the payload variable.
    resolvedConcolicVariables->emplace(payLoadVar, payloadExpr);
    return payloadExpr;
}

big_int Bmv2Concolic::computeChecksum(const std::vector<const IR::Expression*>& exprList,
                                      const Model* completedModel, int algo,
                                      Model::ExpressionMap* resolvedExpressions) {
    // Pick a checksum according to the algorithm value.
    ChecksumFunction checksumFun = nullptr;
    switch (algo) {
        case Bmv2HashAlgorithm::csum16: {
            checksumFun = BMv2Hash::csum16;
            break;
        }
        case Bmv2HashAlgorithm::crc32: {
            checksumFun = BMv2Hash::crc32;
            break;
        }
        case Bmv2HashAlgorithm::crc16: {
            checksumFun = BMv2Hash::crc16;
            break;
        }
        case Bmv2HashAlgorithm::identity: {
            checksumFun = BMv2Hash::identity;
            break;
        }
        case Bmv2HashAlgorithm::xor16: {
            checksumFun = BMv2Hash::xor16;
            break;
        }
        case Bmv2HashAlgorithm::random: {
            BUG("Random should not be encountered here");
        }
        case Bmv2HashAlgorithm::crc32_custom:
        case Bmv2HashAlgorithm::crc16_custom:
        default:
            TESTGEN_UNIMPLEMENTED("Algorithm %1% not implemented for hash.", algo);
    }

    std::vector<char> bytes;
    if (!exprList.empty()) {
        const auto* concatExpr = exprList.at(0);
        for (size_t idx = 1; idx < exprList.size(); idx++) {
            const auto* expr = exprList.at(idx);
            const auto* newWidth =
                IRUtils::getBitType(concatExpr->type->width_bits() + expr->type->width_bits());
            concatExpr = new IR::Concat(newWidth, concatExpr, expr);
        }

        auto concatWidth = concatExpr->type->width_bits();
        auto remainder = concatExpr->type->width_bits() % CHUNK_SIZE;
        // The behavioral model pads widths less than CHUNK_SIZE from the right side.
        // If we have a remainder, add another concatenation here.
        if (remainder != 0) {
            auto fillWidth = CHUNK_SIZE - remainder;
            concatWidth += fillWidth;
            const auto* remainderExpr = IRUtils::getConstant(IRUtils::getBitType(fillWidth), 0);
            concatExpr =
                new IR::Concat(IRUtils::getBitType(concatWidth), concatExpr, remainderExpr);
        }
        auto dataInt = IRUtils::getBigIntFromLiteral(
            completedModel->evaluate(concatExpr, resolvedExpressions));
        bytes = convertBigIntToBytes(dataInt, concatWidth);
    }
    return checksumFun(bytes.data(), bytes.size());
}

const ConcolicMethodImpls::ImplList Bmv2Concolic::Bmv2ConcolicMethodImpls{
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
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable* var,
        const ExecutionState& /*state*/, const Model* completedModel,
        ConcolicVariableMap* resolvedConcolicVariables) {
         const auto* args = var->arguments;
         const auto* checksumVar = args->at(0)->expression;
         if (!(checksumVar->is<IR::Member>() || checksumVar->is<IR::PathExpression>())) {
             TESTGEN_UNIMPLEMENTED("Checksum input %1% of type %2% not supported", checksumVar,
                                   checksumVar->node_type_name());
         }
         // Assign arguments to concrete variables and perform type checking.
         auto algo = args->at(1)->expression->checkedTo<IR::Constant>()->asInt();
         Model::ExpressionMap resolvedExpressions;
         const auto* base = completedModel->evaluate(args->at(2)->expression, &resolvedExpressions);
         auto baseInt = IRUtils::getBigIntFromLiteral(base);
         const auto* dataExpr = args->at(3)->expression;
         const auto* maxHash =
             completedModel->evaluate(args->at(4)->expression, &resolvedExpressions);
         auto maxHashInt = IRUtils::getBigIntFromLiteral(maxHash);

         /// Iterate through the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto* dataList = dataExpr->checkedTo<IR::StructExpression>();
         const auto exprList = IRUtils::flattenStructExpression(dataList);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         if (maxHashInt == 0) {
             computedResult = baseInt;
         } else {
             computedResult = computeChecksum(exprList, completedModel, algo, &resolvedExpressions);
             // Behavioral model uses this technique to limit the hash output.
             computedResult = (baseInt + (computedResult % maxHashInt));
         }

         // Assign a value to the @param result using the computed result
         if (const auto* checksumVarType = checksumVar->type->to<IR::Type_Bits>()) {
             auto concolicMember = IRUtils::getConcolicMember(var, 0);
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[concolicMember] =
                 IRUtils::getConstant(checksumVarType, computedResult);

         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVar->type);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         for (const auto& variable : resolvedExpressions) {
             const auto* varName = variable.first;
             const auto* varExpr = variable.second;
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
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable* var,
        const ExecutionState& /*state*/, const Model* completedModel,
        ConcolicVariableMap* resolvedConcolicVariables) {
         // Assign arguments to concrete variables and perform type checking.
         const auto* args = var->arguments;
         const auto* checksumVar = args->at(0)->expression;
         auto algo = args->at(1)->expression->checkedTo<IR::Constant>()->asInt();
         const auto* dataExpr = args->at(2)->expression;
         const auto* checksumVarType = checksumVar->type;
         // This is the maximum value this checksum can have.
         auto maxHashInt = IRUtils::getMaxBvVal(checksumVarType);

         /// Iterate through the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto* dataList = dataExpr->checkedTo<IR::StructExpression>();
         const std::vector<const IR::Expression*> exprList =
             IRUtils::flattenStructExpression(dataList);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         Model::ExpressionMap resolvedExpressions;
         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         computedResult = computeChecksum(exprList, completedModel, algo, &resolvedExpressions);
         // Behavioral model uses this technique to limit the checksum output.
         computedResult = std::min(computedResult, maxHashInt);
         // Assign a value to the @param result using the computed result
         if (checksumVarType->is<IR::Type_Bits>()) {
             auto concolicMember = IRUtils::getConcolicMember(var, 0);
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[concolicMember] =
                 IRUtils::getConstant(checksumVarType, computedResult);
         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVarType);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         for (const auto& variable : resolvedExpressions) {
             const auto* varName = variable.first;
             const auto* varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},

    {"*method_checksum_with_payload",
     {"result", "algo", "data"},
     [](cstring /*concolicMethodName*/, const IR::ConcolicVariable* var,
        const ExecutionState& state, const Model* completedModel,
        ConcolicVariableMap* resolvedConcolicVariables) {
         // Assign arguments to concrete variables and perform type checking.
         const auto* args = var->arguments;
         const auto* checksumVar = args->at(0)->expression;
         auto algo = args->at(1)->expression->checkedTo<IR::Constant>()->asInt();
         const auto* dataExpr = args->at(2)->expression;
         const auto* checksumVarType = checksumVar->type;
         Model::ExpressionMap resolvedExpressions;

         /// Iterate through the data input and compute the byte array that will be used for
         /// checksum computation.
         // We only support struct expressions as argument input for now.
         const auto* data = dataExpr->checkedTo<IR::StructExpression>();
         auto exprList = IRUtils::flattenStructExpression(data);
         if (exprList.empty()) {
             TESTGEN_UNIMPLEMENTED("Data input is empty. This case is not implemented.");
         }

         // This is the maximum value this checksum can have.
         auto maxHashInt = IRUtils::getMaxBvVal(checksumVarType);
         const auto& packetBitSizeVar = ExecutionState::getInputPacketSizeVar();
         const auto* payloadSizeConst = completedModel->evaluate(packetBitSizeVar);
         int calculatedPacketSize = IRUtils::getIntFromLiteral(payloadSizeConst);

         const auto* inputPacketExpr = state.getInputPacket();
         int payloadSize = calculatedPacketSize - inputPacketExpr->type->width_bits();
         // If the payload is not 0, we need to add it to our checksum calculation.
         if (payloadSize > 0) {
             const auto* payloadExpr =
                 setAndComputePayload(*completedModel, resolvedConcolicVariables, payloadSize);
             // Fix the payload size only if is not fixed already.
             resolvedConcolicVariables->emplace(packetBitSizeVar, payloadSizeConst);
             exprList.push_back(payloadExpr);
         }

         big_int computedResult = 0;
         // If max is 0, the value written to computedResult will always be base.
         computedResult = computeChecksum(exprList, completedModel, algo, &resolvedExpressions);
         // Behavioral model uses this technique to limit the checksum output.
         computedResult = std::min(computedResult, maxHashInt);
         // Assign a value to the @param result using the computed result
         if (checksumVarType->is<IR::Type_Bits>()) {
             auto concolicMember = IRUtils::getConcolicMember(var, 0);
             // Overwrite any previous assignment or result.
             (*resolvedConcolicVariables)[concolicMember] =
                 IRUtils::getConstant(checksumVarType, computedResult);
         } else {
             TESTGEN_UNIMPLEMENTED("Checksum output %1% of type %2% not supported", checksumVar,
                                   checksumVarType);
         }
         // Generated equations for all the variables that were assigned a value in this iteration
         // of concolic execution.
         for (const auto& variable : resolvedExpressions) {
             const auto* varName = variable.first;
             const auto* varExpr = variable.second;
             (*resolvedConcolicVariables)[varName] = varExpr;
         }
     }},
};

const ConcolicMethodImpls::ImplList* Bmv2Concolic::getBmv2ConcolicMethodImpls() {
    return &Bmv2ConcolicMethodImpls;
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools