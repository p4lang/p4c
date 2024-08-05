#include "backends/p4tools/modules/smith/targets/nic/expressions.h"

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/scope.h"

namespace P4Tools::P4Smith {

const IR::Type_Bits *NicExpressionGenerator::genBitType(bool isSigned) {
    auto size = Utils::getRandInt(0, sizeof(NicExpressionGenerator::BIT_WIDTHS) / sizeof(int) - 1);
    return IR::Type_Bits::get(NicExpressionGenerator::BIT_WIDTHS[size], isSigned);
}

const IR::Type *NicExpressionGenerator::pickRndBaseType(const std::vector<int64_t> &type_probs) {
    if (type_probs.size() != 7) {
        BUG("pickRndBaseType: Type probabilities must be exact");
    }
    const IR::Type *tb = nullptr;
    switch (Utils::getRandInt(type_probs)) {
        case 0: {
            // bool
            tb = genBoolType();
            break;
        }
        case 1: {
            // error, this is not supported right now
            break;
        }
        case 2: {
            // int, this is not supported right now
            tb = genIntType();
            break;
        }
        case 3: {
            // string, this is not supported right now
            break;
        }
        case 4: {
            // bit<>
            tb = genBitType(false);
            break;
        }
        case 5: {
            // int<>
            tb = genBitType(true);
            break;
        }
        case 6: {
            // varbit<>, this is not supported right now
            break;
        }
    }
    return tb;
}

const IR::Type *NicExpressionGenerator::pickRndType(TyperefProbs type_probs) {
    const std::vector<int64_t> &typeProbsVector = {
        type_probs.p4_bit,          type_probs.p4_signed_bit, type_probs.p4_varbit,
        type_probs.p4_int,          type_probs.p4_error,      type_probs.p4_bool,
        type_probs.p4_string,       type_probs.p4_enum,       type_probs.p4_header,
        type_probs.p4_header_stack, type_probs.p4_struct,     type_probs.p4_header_union,
        type_probs.p4_tuple,        type_probs.p4_void,       type_probs.p4_match_kind};

    const std::vector<int64_t> &basetypeProbs = {
        type_probs.p4_bool, type_probs.p4_error,      type_probs.p4_int,   type_probs.p4_string,
        type_probs.p4_bit,  type_probs.p4_signed_bit, type_probs.p4_varbit};

    if (typeProbsVector.size() != 15) {
        BUG("pickRndType: Type probabilities must be exact");
    }
    const IR::Type *tp = nullptr;
    size_t idx = Utils::getRandInt(typeProbsVector);
    switch (idx) {
        case 0: {
            // bit<>
            tp = genBitType(false);
            break;
        }
        case 1: {
            // int<>
            tp = genBitType(true);
            break;
        }
        case 2: {
            // varbit<>, this is not supported right now
            break;
        }
        case 3: {
            tp = ExpressionGenerator::genIntType();
            break;
        }
        case 4: {
            // error, this is not supported right now
            break;
        }
        case 5: {
            // bool
            tp = ExpressionGenerator::genBoolType();
            break;
        }
        case 6: {
            // string, this is not supported right now
            break;
        }
        case 7: {
            // enum, this is not supported right now
            break;
        }
        case 8: {
            // header
            auto lTypes = P4Scope::getDecls<IR::Type_Header>();
            if (lTypes.empty()) {
                tp = pickRndBaseType(basetypeProbs);
                break;
            }
            const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
            auto typeName = candidateType->name.name;
            // check if struct is forbidden
            if (P4Scope::notInitializedStructs.count(typeName) == 0) {
                tp = new IR::Type_Name(candidateType->name.name);
            } else {
                tp = pickRndBaseType(basetypeProbs);
            }
            break;
        }
        case 9: {
            tp = target().declarationGenerator().genHeaderStackType();
            break;
        }
        case 10: {
            // struct
            auto lTypes = P4Scope::getDecls<IR::Type_Struct>();
            if (lTypes.empty()) {
                tp = pickRndBaseType(basetypeProbs);
                break;
            }
            const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
            auto typeName = candidateType->name.name;
            // check if struct is forbidden
            if (P4Scope::notInitializedStructs.count(typeName) == 0) {
                tp = new IR::Type_Name(candidateType->name.name);
            } else {
                tp = pickRndBaseType(basetypeProbs);
            }
            break;
        }
        case 11: {
            // header union, this is not supported right now
            break;
        }
        case 12: {
            // tuple, this is not supported right now
            break;
        }
        case 13: {
            // void
            tp = IR::Type_Void::get();
            break;
        }
        case 14: {
            // match kind, this is not supported right now
            break;
        }
    }
    if (tp == nullptr) {
        BUG("pickRndType: Chosen type is Null!");
    }

    return tp;
}

}  // namespace P4Tools::P4Smith