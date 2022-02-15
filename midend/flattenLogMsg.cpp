#include "midend/flattenLogMsg.h"
#include "flattenLogMsg.h"

namespace P4 {

FlattenLogMsg::FlattenLogMsg(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
    refMap(refMap), typeMap(typeMap) {
}

TypeLogMsgParams FlattenLogMsg::unfoldStruct(const IR::Expression* expr, std::string strParam,
                                             std::string curName) {
    TypeLogMsgParams result;
    auto* exprType = typeMap->getType(expr, true);
    if (!expr->is<IR::StructExpression>()) {
        std::cout << exprType << std::endl;
        if (auto structType = exprType->to<IR::Type_StructLike>()) {
            result.second += "(";
            for (auto field : structType->fields) {
                std::string nm = field->name.name + std::string(":");
                std::cout << expr << ":" << typeMap->getType(expr) << std::endl;
                auto* newMember = new IR::Member(field->type, expr, field->name);
                if (field->type->is<IR::Type_StructLike>()) {
                    auto curResult = unfoldStruct(newMember, strParam, field->name.name.c_str());
                    nm += curResult.second;
                    result.first.insert(result.first.end(), curResult.first.begin(),
                                        curResult.first.end());
                } else {
                    result.first.push_back(new IR::NamedExpression(expr->srcInfo, newName(),
                                                                   newMember));
                    nm += std::string("{}");
                }
                if (result.second.length() > 1)
                    result.second += ",";
                result.second += nm;
            }
            result.second += std::string(")");
            return result;
        }
        result.first.push_back(new IR::NamedExpression(expr->srcInfo, newName(), expr));
        result.second = curName + std::string(" : {}");
        return result;
    }
    auto* structExpr = expr->to<IR::StructExpression>();
    std::string strResult;
    for (auto namedExpr : structExpr->components) {
        std::cout << namedExpr->name << ":" << namedExpr->expression << ":" << namedExpr->expression->node_type_name() << std::endl;
        size_t n = strParam.find("{}");
        if (n != std::string::npos) {
            strResult += strParam.substr(0, n);
            strParam = strParam.substr(n + 2); 
        }
        exprType = typeMap->getType(namedExpr->expression, true);
        std::cout << exprType << std::endl;
        if (exprType->is<IR::Type_StructLike>()) {
            std::string nm;
            if (curName.length()) {
                nm = curName + std::string(".");
            }
            nm += namedExpr->name.name.c_str();
            auto innerFields = unfoldStruct(namedExpr->expression, "", nm);
            result.first.insert(result.first.end(), innerFields.first.begin(), innerFields.first.end());
            strResult += innerFields.second;
        } else {
            strResult += "{}";
            result.first.push_back(
                new IR::NamedExpression(namedExpr->srcInfo, newName(), namedExpr->expression));
        }
    }
    result.second = strResult + strParam;
    return result;
}

IR::ID FlattenLogMsg::newName() {
    std::ostringstream ostr;
    ostr << "f" << index++;
    return IR::ID(ostr.str());
}

const IR::Type_StructLike* FlattenLogMsg::generateNewStructType(
    const IR::Type_StructLike* structType, IR::IndexedVector<IR::NamedExpression> &v) {
    IR::IndexedVector<IR::StructField> fields;
    for (auto namedExpr : v) {
        fields.push_back(new IR::StructField(namedExpr->srcInfo, namedExpr->name,
                                             namedExpr->expression->type));
    }
    return new IR::Type_Struct(structType->srcInfo,
        IR::ID(refMap->newName(structType->name.originalName)), fields);
}

const IR::Node* FlattenLogMsg::postorder(IR::MethodCallStatement* methodCallStatement) {
    if (auto* path = methodCallStatement->methodCall->method->to<IR::PathExpression>()) {
        if (path->path->name.name == "log_msg") {
            auto* param1 = methodCallStatement->methodCall->arguments->at(0)->expression;
            auto* param2 = methodCallStatement->methodCall->arguments->at(1)->expression;
            if (!param1->is<IR::StringLiteral>() || !param2->is<IR::StructExpression>()) {
                return methodCallStatement;
            }
            std::cout << "call : " << methodCallStatement << std::endl;
            std::string strParam1 = param1->to<IR::StringLiteral>()->value.c_str();
            auto* argType =
                typeMap->getTypeType(methodCallStatement->methodCall->typeArguments->at(0), true);
            std::cout << argType << " : " << argType->node_type_name() << std::endl;
            if (argType->is<IR::Type_StructLike>()) {
                std::cout << methodCallStatement->methodCall->arguments->at(0)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(0)->expression->type << std::endl;
                std::cout << methodCallStatement->methodCall->arguments->at(1)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(1)->expression->type << std::endl;
                auto* structType = argType->to<IR::Type_StructLike>();
                index = 0;
                auto exprVector = 
                    unfoldStruct(methodCallStatement->methodCall->arguments->at(1)->expression,
                                 strParam1);
                if (structType->fields.size() == exprVector.first.size()) {
                    return methodCallStatement;
                }
                structType = generateNewStructType(structType, exprVector.first);
                auto* newMethodCall = methodCallStatement->methodCall->clone();
                auto* newTypeArguments = newMethodCall->typeArguments->clone();
                newTypeArguments->clear();
                newTypeArguments->push_back(structType);
                newMethodCall->typeArguments = newTypeArguments;
                auto* newArguments = newMethodCall->arguments->clone();
                auto* newArgument = newMethodCall->arguments->at(0)->clone();
                auto* newString = newArgument->expression->to<IR::StringLiteral>()->clone();
                newString->value = exprVector.second;
                newArgument->expression = newString;
                newArguments->at(0) = newArgument;
                newArgument = newMethodCall->arguments->at(1)->clone();
                auto* oldType = newArgument->expression->to<IR::StructExpression>()->structType;
                auto newStructExpression =new IR::StructExpression(oldType->srcInfo, structType,
                    new IR::Type_Name(structType->name), exprVector.first);
                newStructExpression->components = exprVector.first;
                typeMap->setType(structType, structType);
                typeMap->setType(newStructExpression, structType);
                typeMap->setType(new IR::Type_Name(structType->name), structType);
                refMap->setDeclaration(new IR::Path(structType->name), structType);
                newArgument->expression = newStructExpression;
                newArguments->at(1) = newArgument;
                std::cout << newArguments->at(1)->expression->type << std::endl;
                newMethodCall->arguments = newArguments;
                std::cout << newMethodCall->arguments->at(1)->expression->type << std::endl;
                auto* newMethodCallStatement = methodCallStatement->clone();
                newMethodCallStatement->methodCall = newMethodCall;
                std::cout << "new st : " << newMethodCallStatement << std::endl;
                std::cout << newMethodCallStatement->methodCall->arguments->at(1)->expression->type << std::endl;
                return newMethodCallStatement;
            }
        }
    }
    return methodCallStatement;
}

}  // namespace P4
