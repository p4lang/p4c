#include "backends/p4tools/modules/smith/common/declarations.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4::P4Tools::P4Smith {

IR::StatOrDecl *DeclarationGenerator::generateRandomStatementOrDeclaration(bool is_in_func) {
    std::vector<int64_t> percent = {Probabilities::get().STATEMENTORDECLARATION_VAR,
                                    Probabilities::get().STATEMENTORDECLARATION_CONSTANT,
                                    Probabilities::get().STATEMENTORDECLARATION_STATEMENT};
    auto val = Utils::getRandInt(percent);
    if (val == 0) {
        auto *stmt = target().declarationGenerator().genVariableDeclaration();
        if (stmt == nullptr) {
            BUG("Declaration in statementOrDeclaration should not be nullptr!");
        }
        return stmt;
    }
    if (val == 1) {
        auto *stmt = target().declarationGenerator().genConstantDeclaration();
        if (stmt == nullptr) {
            BUG("Declaration in statementOrDeclaration should not be nullptr!");
        }
        return stmt;
    }
    auto genStmt = target().statementGenerator();
    auto *stmt = genStmt.genAssignmentOrMethodCallStatement(is_in_func);
    if (stmt == nullptr) {
        // it can happen that no statement can be generated
        // for example in functions without writable values
        // so declare a variable instead
        return target().declarationGenerator().genVariableDeclaration();
    }
    return stmt;
}

IR::Annotations *DeclarationGenerator::genAnnotation() {
    Util::SourceInfo si;
    IR::Vector<IR::Annotation> annotations;
    IR::Vector<IR::Expression> exprs;
    cstring name = IR::Annotation::nameAnnotation;
    auto *strLiteral = new IR::StringLiteral(IR::Type_String::get(), getRandomString(6));

    exprs.push_back(strLiteral);

    auto *annotation = new IR::Annotation(si, name, exprs, false);
    annotations.push_back(annotation);

    return new IR::Annotations(annotations);
}

IR::Declaration_Constant *DeclarationGenerator::genConstantDeclaration() {
    cstring name = getRandomString(6);
    TyperefProbs typePercent = {
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_BIT,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_INT,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_ERROR,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_BOOL,
        Probabilities::get().CONSTANTDECLARATION_BASETYPE_STRING,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_ENUM,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_HEADER,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_STRUCT,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().CONSTANTDECLARATION_DERIVED_TUPLE,
        Probabilities::get().CONSTANTDECLARATION_TYPE_VOID,
        Probabilities::get().CONSTANTDECLARATION_TYPE_MATCH_KIND,
    };

    const auto *tp = target().expressionGenerator().pickRndType(typePercent);

    IR::Declaration_Constant *ret = nullptr;
    // constant declarations need to be compile-time known
    P4Scope::req.compile_time_known = true;

    if (tp->is<IR::Type_Bits>() || tp->is<IR::Type_InfInt>() || tp->is<IR::Type_Boolean>() ||
        tp->is<IR::Type_Name>()) {
        auto *expr = target().expressionGenerator().genExpression(tp);
        ret = new IR::Declaration_Constant(name, tp, expr);
    } else {
        BUG("Type %s not supported!", tp->node_type_name());
    }
    P4Scope::req.compile_time_known = false;

    P4Scope::addToScope(ret);

    return ret;
}

IR::P4Action *DeclarationGenerator::genActionDeclaration() {
    cstring name = getRandomString(5);
    IR::ParameterList *params = nullptr;
    IR::BlockStatement *blk = nullptr;
    P4Scope::startLocalScope();
    P4Scope::prop.in_action = true;
    params = genParameterList();

    blk = target().statementGenerator().genBlockStatement(false);

    auto *ret = new IR::P4Action(name, params, blk);

    P4Scope::prop.in_action = false;
    P4Scope::endLocalScope();

    P4Scope::addToScope(ret);

    return ret;
}

IR::IndexedVector<IR::Declaration> DeclarationGenerator::genLocalControlDecls() {
    IR::IndexedVector<IR::Declaration> localDecls;

    auto vars = Utils::getRandInt(Declarations::get().MIN_VAR, Declarations::get().MAX_VAR);
    auto decls =
        Utils::getRandInt(Declarations::get().MIN_INSTANCE, Declarations::get().MAX_INSTANCE);
    auto actions =
        Utils::getRandInt(Declarations::get().MIN_ACTION, Declarations::get().MAX_ACTION);
    auto tables = Utils::getRandInt(Declarations::get().MIN_TABLE, Declarations::get().MAX_TABLE);

    // variableDeclarations
    for (int i = 0; i <= vars; i++) {
        auto *varDecl = genVariableDeclaration();
        localDecls.push_back(varDecl);
    }

    // declaration_instance
    for (int i = 0; i <= decls; i++) {
        auto *declIns = genControlDeclarationInstance();

        if (declIns == nullptr) {
            continue;
        }
        localDecls.push_back(declIns);
    }

    // actionDeclarations
    for (int i = 0; i <= actions; i++) {
        auto *actDecl = genActionDeclaration();
        localDecls.push_back(actDecl);
    }

    for (int i = 0; i <= tables; i++) {
        auto *tabDecl = target().tableGenerator().genTableDeclaration();
        localDecls.push_back(tabDecl);
    }
    return localDecls;
    // instantiations
}

IR::P4Control *DeclarationGenerator::genControlDeclaration() {
    // start of new scope
    P4Scope::startLocalScope();
    cstring name = getRandomString(7);
    IR::ParameterList *params = genParameterList();
    auto *typeCtrl = new IR::Type_Control(name, params);

    IR::IndexedVector<IR::Declaration> localDecls = genLocalControlDecls();
    // apply body
    auto *applyBlock = target().statementGenerator().genBlockStatement(false);

    // end of scope
    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4ctrl = new IR::P4Control(name, typeCtrl, localDecls, applyBlock);
    P4Scope::addToScope(p4ctrl);

    return p4ctrl;
}

IR::Declaration_Instance *DeclarationGenerator::genControlDeclarationInstance() {
    auto p4Ctrls = P4Scope::getDecls<IR::P4Control>();
    size_t size = p4Ctrls.size();

    if (size == 0) {
        // FIXME: Figure out a better way to handle this nullptr
        return nullptr;
    }
    auto *args = new IR::Vector<IR::Argument>();
    const IR::P4Control *p4ctrl = p4Ctrls.at(Utils::getRandInt(0, size - 1));
    IR::Type *tp = new IR::Type_Name(p4ctrl->name);
    auto *decl = new IR::Declaration_Instance(cstring(getRandomString(6)), tp, args);
    P4Scope::addToScope(decl);
    return decl;
}

IR::Type *DeclarationGenerator::genDerivedTypeDeclaration() { return genHeaderTypeDeclaration(); }

IR::IndexedVector<IR::Declaration_ID> DeclarationGenerator::genIdentifierList(size_t len) {
    IR::IndexedVector<IR::Declaration_ID> declIds;
    std::set<cstring> declIdsName;

    for (size_t i = 0; i < len; i++) {
        cstring name = getRandomString(2);
        auto *declId = new IR::Declaration_ID(name);

        if (declIdsName.find(name) != declIdsName.end()) {
            delete name;
            delete declId;
            continue;
        }

        declIds.push_back(declId);
    }

    return declIds;
}

IR::IndexedVector<IR::SerEnumMember> DeclarationGenerator::genSpecifiedIdentifier(size_t len) {
    IR::IndexedVector<IR::SerEnumMember> members;
    std::set<cstring> membersName;

    for (size_t i = 0; i < len; i++) {
        cstring name = getRandomString(2);
        IR::Expression *ex = P4Tools::P4Smith::ExpressionGenerator::genIntLiteral();

        if (membersName.find(name) != membersName.end()) {
            delete ex;
            continue;
        }

        auto *member = new IR::SerEnumMember(name, ex);

        members.push_back(member);
    }

    return members;
}
IR::IndexedVector<IR::SerEnumMember> DeclarationGenerator::genSpecifiedIdentifierList(size_t len) {
    IR::IndexedVector<IR::SerEnumMember> members;
    std::set<cstring> membersName;

    for (size_t i = 0; i < len; i++) {
        cstring name = getRandomString(2);
        IR::Expression *ex = target().expressionGenerator().genIntLiteral();

        if (membersName.find(name) != membersName.end()) {
            delete ex;
            continue;
        }

        auto *member = new IR::SerEnumMember(name, ex);

        members.push_back(member);
    }

    return members;
}

IR::Type_Enum *DeclarationGenerator::genEnumDeclaration(cstring name) {
    auto declIds = genIdentifierList(3);
    auto *ret = new IR::Type_Enum(name, declIds);

    P4Scope::addToScope(ret);
    return ret;
}

IR::Type_SerEnum *DeclarationGenerator::genSerEnumDeclaration(cstring name) {
    auto members = genSpecifiedIdentifierList(3);
    const auto *tp = target().expressionGenerator().genBitType(false);

    auto *ret = new IR::Type_SerEnum(name, tp, members);

    P4Scope::addToScope(ret);
    return ret;
}

IR::Type *DeclarationGenerator::genEnumTypeDeclaration(int type) {
    cstring name = getRandomString(4);
    if (type == 0) {
        return genEnumDeclaration(name);
    }
    return genSerEnumDeclaration(name);
}

IR::Method *DeclarationGenerator::genExternDeclaration() {
    cstring name = getRandomString(7);
    IR::Type_Method *tm = nullptr;
    P4Scope::startLocalScope();
    IR::ParameterList *params = genParameterList();

    // externs have the same type restrictions as functions
    TyperefProbs typePercent = {
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_BIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_INT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_ERROR,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_BOOL,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_STRING,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_ENUM,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_STRUCT,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_TUPLE,
        Probabilities::get().FUNCTIONDECLARATION_TYPE_VOID,
        Probabilities::get().FUNCTIONDECLARATION_TYPE_MATCH_KIND,
    };
    const auto *returnType = target().expressionGenerator().pickRndType(typePercent);
    tm = new IR::Type_Method(returnType, params, name);
    auto *ret = new IR::Method(name, tm);
    P4Scope::endLocalScope();
    P4Scope::addToScope(ret);
    return ret;
}

IR::Function *DeclarationGenerator::genFunctionDeclaration() {
    cstring name = getRandomString(7);
    IR::Type_Method *tm = nullptr;
    IR::BlockStatement *blk = nullptr;
    P4Scope::startLocalScope();
    IR::ParameterList *params = genParameterList();

    TyperefProbs typePercent = {
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_BIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_INT,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_ERROR,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_BOOL,
        Probabilities::get().FUNCTIONDECLARATION_BASETYPE_STRING,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_ENUM,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_STRUCT,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().FUNCTIONDECLARATION_DERIVED_TUPLE,
        Probabilities::get().FUNCTIONDECLARATION_TYPE_VOID,
        Probabilities::get().FUNCTIONDECLARATION_TYPE_MATCH_KIND,
    };
    const auto *returnType = target().expressionGenerator().pickRndType(typePercent);
    tm = new IR::Type_Method(returnType, params, name);

    P4Scope::prop.ret_type = returnType;
    blk = target().statementGenerator().genBlockStatement(true);
    P4Scope::prop.ret_type = nullptr;

    auto *ret = new IR::Function(name, tm, blk);
    P4Scope::endLocalScope();
    P4Scope::addToScope(ret);
    return ret;
}

IR::Type_Header *DeclarationGenerator::genEthernetHeaderType() {
    IR::IndexedVector<IR::StructField> fields;
    auto *ethDst = new IR::StructField("dst_addr", IR::Type_Bits::get(48, false));
    auto *ethSrc = new IR::StructField("src_addr", IR::Type_Bits::get(48, false));
    auto *ethType = new IR::StructField("eth_type", IR::Type_Bits::get(16, false));

    fields.push_back(ethDst);
    fields.push_back(ethSrc);
    fields.push_back(ethType);

    auto *ret = new IR::Type_Header(IR::ID(ETH_HEADER_T), fields);
    P4Scope::addToScope(ret);

    return ret;
}

IR::Type_Header *DeclarationGenerator::genHeaderTypeDeclaration() {
    cstring name = getRandomString(6);
    IR::IndexedVector<IR::StructField> fields;
    TyperefProbs typePercent = {
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_BIT,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_INT,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_ERROR,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_BOOL,
        Probabilities::get().HEADERTYPEDECLARATION_BASETYPE_STRING,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_ENUM,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_HEADER,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_STRUCT,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().HEADERTYPEDECLARATION_DERIVED_TUPLE,
        Probabilities::get().HEADERTYPEDECLARATION_TYPE_VOID,
        Probabilities::get().HEADERTYPEDECLARATION_TYPE_MATCH_KIND,
    };

    size_t len = Utils::getRandInt(1, 5);
    for (size_t i = 0; i < len; i++) {
        cstring fieldName = getRandomString(4);
        const auto *fieldTp = target().expressionGenerator().pickRndType(typePercent);

        if (const auto *structTp = fieldTp->to<IR::Type_Struct>()) {
            fieldTp = new IR::Type_Name(structTp->name);
        }
        auto *sf = new IR::StructField(fieldName, fieldTp);
        fields.push_back(sf);
    }
    auto *ret = new IR::Type_Header(name, fields);
    if (P4Scope::req.byte_align_headers) {
        auto remainder = ret->width_bits() % 8;
        if (remainder != 0) {
            const auto *padBit = IR::Type_Bits::get(8 - remainder, false);
            auto *padField = new IR::StructField("padding", padBit);
            ret->fields.push_back(padField);
        }
    }
    P4Scope::addToScope(ret);

    return ret;
}

IR::Type_HeaderUnion *DeclarationGenerator::genHeaderUnionDeclaration() {
    cstring name = getRandomString(6);

    IR::IndexedVector<IR::StructField> fields;
    auto lTypes = P4Scope::getDecls<IR::Type_Header>();
    if (lTypes.size() < 2) {
        BUG("Creating a header union assumes at least two headers!");
    }
    // !sure if this correct...
    size_t len = Utils::getRandInt(2, lTypes.size() - 2);
    std::set<cstring> visitedHeaders;
    // we need to guarantee correct execution so try as long as we can
    // this is a bit dicey... do !like it
    int attempts = 0;
    while (true) {
        if (attempts >= 100) {
            BUG("We should not need this many attempts!");
        }
        attempts++;
        cstring fieldName = getRandomString(4);
        const auto *hdrTp = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
        // check if we have already added this header
        if (visitedHeaders.find(hdrTp->name) != visitedHeaders.end()) {
            continue;
        }
        auto *tpName = new IR::Type_Name(hdrTp->name);
        auto *sf = new IR::StructField(fieldName, tpName);
        visitedHeaders.insert(hdrTp->name);
        fields.push_back(sf);
        if (fields.size() == len) {
            break;
        }
    }

    auto *ret = new IR::Type_HeaderUnion(name, fields);

    P4Scope::addToScope(ret);

    return ret;
}

IR::Type *DeclarationGenerator::genHeaderStackType() {
    auto lTypes = P4Scope::getDecls<IR::Type_Header>();
    if (lTypes.empty()) {
        BUG("Creating a header stacks assumes at least one declared header!");
    }
    const auto *hdrTp = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
    auto stackSize = Utils::getRandInt(1, MAX_HEADER_STACK_SIZE);
    auto *hdrTypeName = new IR::Type_Name(hdrTp->name);
    auto *ret =
        new IR::Type_Stack(hdrTypeName, new IR::Constant(IR::Type_InfInt::get(), stackSize));

    P4Scope::addToScope(ret);

    return ret;
}

IR::Type_Struct *DeclarationGenerator::genStructTypeDeclaration() {
    cstring name = getRandomString(6);

    IR::IndexedVector<IR::StructField> fields;
    TyperefProbs typePercent = {
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_BIT,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_INT,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_ERROR,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_BOOL,
        Probabilities::get().STRUCTTYPEDECLARATION_BASETYPE_STRING,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_ENUM,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_HEADER,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_STRUCT,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().STRUCTTYPEDECLARATION_DERIVED_TUPLE,
        Probabilities::get().STRUCTTYPEDECLARATION_TYPE_VOID,
        Probabilities::get().STRUCTTYPEDECLARATION_TYPE_MATCH_KIND,
    };
    auto lTypes = P4Scope::getDecls<IR::Type_Header>();
    if (lTypes.empty()) {
        return nullptr;
    }
    size_t len = Utils::getRandInt(1, 5);

    for (size_t i = 0; i < len; i++) {
        const auto *fieldTp = target().expressionGenerator().pickRndType(typePercent);
        cstring fieldName = getRandomString(4);
        if (fieldTp->to<IR::Type_Stack>() != nullptr) {
            // Right now there is now way to initialize a header stack
            // So we have to add the entire structure to the banned expressions
            P4Scope::notInitializedStructs.insert(name);
        }
        auto *sf = new IR::StructField(fieldName, fieldTp);
        fields.push_back(sf);
    }

    auto *ret = new IR::Type_Struct(name, fields);

    P4Scope::addToScope(ret);

    return ret;
}

IR::Type_Struct *DeclarationGenerator::genHeaderStruct() {
    IR::IndexedVector<IR::StructField> fields;

    // Tao: hard code for ethernet_t eth_hdr;
    auto *ethSf = new IR::StructField(ETH_HDR, new IR::Type_Name(ETH_HEADER_T));
    fields.push_back(ethSf);

    size_t len = Utils::getRandInt(1, 5);
    // we can only generate very specific types for headers
    // header, header stack, header union
    std::vector<int64_t> percent = {Probabilities::get().STRUCTTYPEDECLARATION_HEADERS_HEADER,
                                    Probabilities::get().STRUCTTYPEDECLARATION_HEADERS_STACK};
    for (size_t i = 0; i < len; i++) {
        cstring fieldName = getRandomString(4);
        IR::Type *tp = nullptr;
        switch (Utils::getRandInt(percent)) {
            case 0: {
                // TODO(fruffy): We have to assume that this works
                auto lTypes = P4Scope::getDecls<IR::Type_Header>();
                if (lTypes.empty()) {
                    BUG("structTypeDeclaration: No available header for Headers!");
                }
                const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
                tp = new IR::Type_Name(candidateType->name.name);
                break;
            }
            case 1: {
                tp = genHeaderStackType();
                // Right now there is now way to initialize a header stack
                // So we have to add the entire structure to the banned expressions
                P4Scope::notInitializedStructs.insert(cstring("Headers"));
            }
        }
        fields.push_back(new IR::StructField(fieldName, tp));
    }
    auto *ret = new IR::Type_Struct("Headers", fields);

    P4Scope::addToScope(ret);

    return ret;
}

IR::Type_Declaration *DeclarationGenerator::genTypeDeclaration() {
    std::vector<int64_t> percent = {Probabilities::get().TYPEDECLARATION_HEADER,
                                    Probabilities::get().TYPEDECLARATION_STRUCT,
                                    Probabilities::get().TYPEDECLARATION_UNION};
    IR::Type_Declaration *decl = nullptr;
    bool useDefaultDecl = false;
    switch (Utils::getRandInt(percent)) {
        case 0: {
            useDefaultDecl = true;
            break;
        }
        case 1: {
            decl = genStructTypeDeclaration();
            break;
        }
        case 2: {
            // header unions are disabled for now, need to fix assignments
            auto hdrs = P4Scope::getDecls<IR::Type_Header>();
            // we can only generate a union if we have at least two headers
            if (hdrs.size() > 1) {
                decl = genHeaderUnionDeclaration();
                if (decl == nullptr) {
                    useDefaultDecl = true;
                }
            } else {
                useDefaultDecl = true;
            }
            break;
        }
    }
    if (useDefaultDecl) {
        decl = genHeaderTypeDeclaration();
    }

    return decl;
}

const IR::Type *DeclarationGenerator::genType() {
    std::vector<int64_t> percent = {Probabilities::get().TYPEDEFDECLARATION_BASE,
                                    Probabilities::get().TYPEDEFDECLARATION_STRUCTLIKE,
                                    Probabilities::get().TYPEDEFDECLARATION_STACK};

    std::vector<int64_t> typeProbs = {Probabilities::get().TYPEDEFDECLARATION_BASETYPE_BOOL,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_ERROR,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_INT,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_STRING,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_BIT,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_SIGNED_BIT,
                                      Probabilities::get().TYPEDEFDECLARATION_BASETYPE_VARBIT};
    const IR::Type *tp = nullptr;
    switch (Utils::getRandInt(percent)) {
        case 0: {
            std::vector<int> bTypes = {1};  // only bit<>
            tp = target().expressionGenerator().pickRndBaseType(typeProbs);
            break;
        }
        case 1: {
            auto lTypes = P4Scope::getDecls<IR::Type_StructLike>();
            if (lTypes.empty()) {
                return nullptr;
            }
            const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
            tp = new IR::Type_Name(candidateType->name.name);
            break;
        }
        case 2: {
            // tp = headerStackType::gen();
            break;
        }
    }
    return tp;
}

IR::Type_Typedef *DeclarationGenerator::genTypeDef() {
    cstring name = getRandomString(5);
    auto *ret = new IR::Type_Typedef(name, genType());
    P4Scope::addToScope(ret);
    return ret;
}

IR::Type_Newtype *DeclarationGenerator::genNewtype() {
    cstring name = getRandomString(5);
    IR::Type *type = nullptr;

    auto *ret = new IR::Type_Newtype(name, type);
    P4Scope::addToScope(ret);
    return ret;
}

IR::Type *DeclarationGenerator::genTypeDefOrNewType() {
    // TODO(fruffy): we only have typedef now, no newtype
    return genTypeDef();
}

IR::Declaration_Variable *DeclarationGenerator::genVariableDeclaration() {
    cstring name = getRandomString(6);

    TyperefProbs typePercent = {
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_BIT,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_SIGNED_BIT,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_VARBIT,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_INT,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_ERROR,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_BOOL,
        Probabilities::get().VARIABLEDECLARATION_BASETYPE_STRING,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_ENUM,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_HEADER,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_HEADER_STACK,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_STRUCT,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_HEADER_UNION,
        Probabilities::get().VARIABLEDECLARATION_DERIVED_TUPLE,
        Probabilities::get().VARIABLEDECLARATION_TYPE_VOID,
        Probabilities::get().VARIABLEDECLARATION_TYPE_MATCH_KIND,
    };

    const IR::Type *tp = target().expressionGenerator().pickRndType(typePercent);

    IR::Declaration_Variable *ret = nullptr;

    if (tp->is<IR::Type_Bits>() || tp->is<IR::Type_InfInt>() || tp->is<IR::Type_Boolean>() ||
        tp->is<IR::Type_Name>()) {
        auto *expr = target().expressionGenerator().genExpression(tp);
        ret = new IR::Declaration_Variable(name, tp, expr);
    } else if (tp->is<IR::Type_Stack>()) {
        // header stacks do !have an initializer yet
        ret = new IR::Declaration_Variable(name, tp);
    } else {
        BUG("Type %s not supported!", tp->node_type_name());
    }

    P4Scope::addToScope(ret);

    return ret;
}

IR::Parameter *DeclarationGenerator::genTypedParameter(bool if_none_dir) {
    cstring name = getRandomString(4);
    const IR::Type *tp = nullptr;
    IR::Direction dir;
    TyperefProbs typePercent;

    if (if_none_dir) {
        typePercent = {
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_BIT,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_SIGNED_BIT,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_VARBIT,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_INT,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_ERROR,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_BOOL,
            Probabilities::get().PARAMETER_NONEDIR_BASETYPE_STRING,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_ENUM,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_HEADER,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_HEADER_STACK,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_STRUCT,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_HEADER_UNION,
            Probabilities::get().PARAMETER_NONEDIR_DERIVED_TUPLE,
            Probabilities::get().PARAMETER_NONEDIR_TYPE_VOID,
            Probabilities::get().PARAMETER_NONEDIR_TYPE_MATCH_KIND,
        };
        dir = IR::Direction::None;
    } else {
        typePercent = {
            Probabilities::get().PARAMETER_BASETYPE_BIT,
            Probabilities::get().PARAMETER_BASETYPE_SIGNED_BIT,
            Probabilities::get().PARAMETER_BASETYPE_VARBIT,
            Probabilities::get().PARAMETER_BASETYPE_INT,
            Probabilities::get().PARAMETER_BASETYPE_ERROR,
            Probabilities::get().PARAMETER_BASETYPE_BOOL,
            Probabilities::get().PARAMETER_BASETYPE_STRING,
            Probabilities::get().PARAMETER_DERIVED_ENUM,
            Probabilities::get().PARAMETER_DERIVED_HEADER,
            Probabilities::get().PARAMETER_DERIVED_HEADER_STACK,
            Probabilities::get().PARAMETER_DERIVED_STRUCT,
            Probabilities::get().PARAMETER_DERIVED_HEADER_UNION,
            Probabilities::get().PARAMETER_DERIVED_TUPLE,
            Probabilities::get().PARAMETER_TYPE_VOID,
            Probabilities::get().PARAMETER_TYPE_MATCH_KIND,
        };
        std::vector<int64_t> dirPercent = {Probabilities::get().PARAMETER_DIR_IN,
                                           Probabilities::get().PARAMETER_DIR_OUT,
                                           Probabilities::get().PARAMETER_DIR_INOUT};
        switch (Utils::getRandInt(dirPercent)) {
            case 0:
                dir = IR::Direction::In;
                break;
            case 1:
                dir = IR::Direction::Out;
                break;
            case 2:
                dir = IR::Direction::InOut;
                break;
            default:
                dir = IR::Direction::None;
        }
    }

    tp = target().expressionGenerator().pickRndType(typePercent);
    return new IR::Parameter(name, dir, tp);
}

IR::Parameter *DeclarationGenerator::genParameter(IR::Direction dir, cstring p_name,
                                                  cstring t_name) {
    IR::Type *tp = new IR::Type_Name(new IR::Path(t_name));
    return new IR::Parameter(p_name, dir, tp);
}

IR::ParameterList *DeclarationGenerator::genParameterList() {
    IR::IndexedVector<IR::Parameter> params;
    size_t totalParams = Utils::getRandInt(0, 3);
    size_t numDirParams = (totalParams != 0U) ? Utils::getRandInt(0, totalParams - 1) : 0;
    size_t numDirectionlessParams = totalParams - numDirParams;
    for (size_t i = 0; i < numDirParams; i++) {
        IR::Parameter *param = genTypedParameter(false);
        if (param == nullptr) {
            BUG("param is null");
        }
        params.push_back(param);
        // add to the scope
        P4Scope::addToScope(param);
        // only add values that are not read-only to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::addLval(param->type, param->name.name, true);
        } else {
            P4Scope::addLval(param->type, param->name.name, false);
        }
    }
    for (size_t i = 0; i < numDirectionlessParams; i++) {
        IR::Parameter *param = genTypedParameter(true);

        if (param == nullptr) {
            BUG("param is null");
        }
        params.push_back(param);
        // add to the scope
        P4Scope::addToScope(param);
        P4Scope::addLval(param->type, param->name.name, true);
    }

    return new IR::ParameterList(params);
}

}  // namespace P4::P4Tools::P4Smith
