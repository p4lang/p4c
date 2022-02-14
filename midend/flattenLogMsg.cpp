#include "midend/flattenLogMsg.h"
#include "flattenLogMsg.h"

namespace P4 {

FlattenLogMsg::FlattenLogMsg(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
    refMap(refMap), typeMap(typeMap) {
}

const IR::Node* FlattenLogMsg::preorder(IR::BlockStatement* blockStatement) {
    IR::IndexedVector<IR::StatOrDecl> components;
    for (auto i : blockStatement->components) {
        FlattenLogMsg flattenLogMsg(refMap, typeMap);
        auto* newStatement = i->apply(flattenLogMsg);
        if (auto* block = newStatement->to<IR::BlockStatement>()) {
            components.insert(components.end(), block->components.begin(),
                              block->components.end());
        } else {
            components.push_back(newStatement->to<IR::StatOrDecl>());
        }
    }
    auto* block = blockStatement->clone();
    block->components = components;
    return block;
}

TypeLogMsgParams FlattenLogMsg::unfoldStruct(const IR::Expression* expr, std::string strParam,
                                             std::string curName) {
    TypeLogMsgParams result;
    if (auto structType = expr->type->to<IR::Type_StructLike>()) {
        result.second += "(";
        for (auto field : structType->fields) {
            std::string nm = curName + std::string(".");
            nm += field->name.name + std::string(" : ");
            auto* newMember = new IR::Member(field->type, expr, field->name);
            if (field->type->is<IR::Type_StructLike>()) {
                auto curResult = unfoldStruct(newMember, strParam, nm);
                nm += curResult.second;
                result.first.insert(result.first.end(), curResult.first.begin(),
                                    curResult.first.end());
            } else {
                nm += "{}";
                result.first.push_back(new IR::NamedExpression(expr->srcInfo, IR::ID(nm),
                                                               newMember));
            }
            result.second += nm + std::string(") ");
        }
        return result;
    }
    if (!expr->is<IR::StructExpression>()) {
        result.first.push_back(new IR::NamedExpression(expr->srcInfo, IR::ID(curName), expr));
        result.second = curName + std::string(" : {}");
        return result;
    }
    auto* structExpr = expr->to<IR::StructExpression>();
    std::string strResult;
    for (auto namedExpr : structExpr->components) {
        std::cout << namedExpr->name << ":" << namedExpr->expression << ":" << namedExpr->expression->type->node_type_name() << std::endl;
        size_t n = strParam.find("{}");
        if (n != std::string::npos) {
            strResult += strParam.substr(0, n);
            strParam = strParam.substr(n + 2); 
        }
        if (namedExpr->expression->type->is<IR::Type_StructLike>()) {
            std::string nm;
            if (curName.length()) {
                nm = curName + std::string(".");
            }
            nm += namedExpr->name.name.c_str();
            auto innerFields = unfoldStruct(namedExpr->expression, "", nm);
            result.first.insert(result.first.end(), innerFields.first.begin(), innerFields.first.end());
            strResult += "(";
            strResult += innerFields.second;
            strResult += ")";
        } else {
            strResult += "{}";
            result.first.push_back(namedExpr);
        }
    }
    result.second = strResult;
    return result;
}

const IR::Type_StructLike* FlattenLogMsg::generateNewStructType(
    const IR::Type_StructLike* structType, IR::IndexedVector<IR::NamedExpression> &v) {
    IR::IndexedVector<IR::StructField> fields;
    size_t index = 1;
    for (auto namedExpr : v) {
        //StructField(Util::SourceInfo srcInfo, IR::ID name, const IR::Type* type);
        std::string nm = std::string("f") + std::to_string(index++);
        fields.push_back(new IR::StructField(namedExpr->srcInfo, IR::ID(nm),
                                             namedExpr->expression->type));
    }
    auto* newStructType = structType->clone();
    newStructType->fields = fields;
    newStructType->name.name = newStructType->name.name + cstring("_1");
    return newStructType;
}

const IR::Node* FlattenLogMsg::postorder(IR::MethodCallStatement* methodCallStatement) {
    if (auto* path = methodCallStatement->methodCall->method->to<IR::PathExpression>()) {
        if (path->path->name.name == "log_msg") {
            auto* param1 = methodCallStatement->methodCall->arguments->at(0)->expression;
            auto* param2 = methodCallStatement->methodCall->arguments->at(1)->expression;
            if (!param1->is<IR::StringLiteral>() || !param2->is<IR::StructExpression>()) {
                return methodCallStatement;
            }
            std::string strParam1 = param1->to<IR::StringLiteral>()->value.c_str();
            auto* argType =
                typeMap->getTypeType(methodCallStatement->methodCall->typeArguments->at(0), true);
            std::cout << argType << " : " << argType->node_type_name() << std::endl;
            if (argType->is<IR::Type_StructLike>()) {
                std::cout << methodCallStatement->methodCall->arguments->at(0)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(0)->expression->type << std::endl;
                std::cout << methodCallStatement->methodCall->arguments->at(1)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(1)->expression->type << std::endl;
                auto* structType = argType->to<IR::Type_StructLike>();
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
                auto newStructExpression = newArgument->expression->to<IR::StructExpression>()->clone();
                newStructExpression->type = structType;
                newStructExpression->components = exprVector.first;
                newArgument->expression = newStructExpression;
                newArguments->at(1) = newArgument;
                newMethodCall->arguments = newArguments;
                auto* newMethodCallStatement = methodCallStatement->clone();
                newMethodCallStatement->methodCall = newMethodCall;
                std::cout << "new st : " << newMethodCallStatement << std::endl;
                return newMethodCallStatement;
            }
        }
    }
    return methodCallStatement;
}

}  // namespace P4
