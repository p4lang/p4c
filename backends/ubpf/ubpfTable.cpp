/*
Copyright 2019 Orange

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <p4/enumInstance.h>
#include "ubpfTable.h"
#include "ubpfType.h"
#include "ubpfParser.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ubpfControl.h"

namespace UBPF {

namespace {

class UbpfActionTranslationVisitor : public EBPF::CodeGenInspector {
 protected:
    const UBPFProgram *program;
    const IR::P4Action *action;
    cstring valueName;

 public:
    UbpfActionTranslationVisitor(cstring valueName, const UBPFProgram *program) :
            EBPF::CodeGenInspector(program->refMap, program->typeMap), program(program),
            action(nullptr), valueName(valueName) {
        CHECK_NULL(program);
    }

    bool preorder(const IR::Member* expression) override {
        cstring name = "";
        if (expression->expr->is<IR::PathExpression>()) {
            name = expression->expr->to<IR::PathExpression>()->path->name.name;
        }
        auto ei = P4::EnumInstance::resolve(expression, typeMap);
        if (ei == nullptr) {
            visit(expression->expr);
            if (name == program->stdMetadataVar) {
                builder->append("->");
            } else {
                builder->append(".");
            }
        }
        builder->append(expression->member);
        return false;
    }

    bool preorder(const IR::PathExpression *expression) override {
        auto decl = program->refMap->getDeclaration(expression->path, true);
        if (decl->is<IR::Parameter>()) {
            auto param = decl->to<IR::Parameter>();
            bool isParam = action->parameters->getParameter(param->name) == param;
            if (isParam) {
                builder->append(valueName);
                builder->append("->u.");
                cstring name = EBPF::EBPFObject::externalName(action);
                builder->append(name);
                builder->append(".");
                builder->append(expression->path->toString());  // original name
                return false;
            }
        }

        visit(expression->path);
        return false;
    }

    bool preorder(const IR::MethodCallExpression *expression) override {
        auto mi = P4::MethodInstance::resolve(expression, refMap,
                                              typeMap);
        auto ef = mi->to<P4::ExternFunction>();
        if (ef != nullptr) {
            if (ef->method->name.name == program->model.drop.name) {
                builder->appendFormat("%s = false", program->control->passVariable);
                return false;
            } else if (ef->method->name.name == program->model.pass.name) {
                builder->appendFormat("%s = true", program->control->passVariable);
                return false;
            } else if (ef->method->name.name ==
                       program->model.ubpf_time_get_ns.name) {
                builder->emitIndent();
                builder->append(program->model.ubpf_time_get_ns.name + "()");
                return false;
            }
        }
        program->control->codeGen->preorder(expression);
        return false;
    }

    void convertActionBody(const IR::Vector<IR::StatOrDecl> *body) {
        for (auto s : *body) {
            if (!s->is<IR::Statement>()) {
                continue;
            } else if (auto block = s->to<IR::BlockStatement>()) {
                convertActionBody(&block->components);
                continue;
            } else if (s->is<IR::ReturnStatement>()) {
                break;
            } else if (s->is<IR::ExitStatement>()) {
                break;
            } else if (s->is<IR::EmptyStatement>()) {
                continue;
            } else if (s->is<IR::MethodCallStatement>()) {
                visit(s);
                builder->newline();
            } else {
                visit(s);
            }
        }
    }

    void convertAction() {
        convertActionBody(&action->body->components);
    }

    bool preorder(const IR::P4Action *act) override {
        action = act;
        convertAction();
        return false;
    }
};  // UbpfActionTranslationVisitor
}  // namespace


UBPFTable::UBPFTable(const UBPFProgram *program,
                     const IR::TableBlock *table,
                     EBPF::CodeGenInspector *codeGen) :
        UBPFTableBase(program, EBPFObject::externalName(table->container),
                      codeGen), table(table) {
    cstring base = instanceName + "_defaultAction";
    defaultActionMapName = program->refMap->newName(base);

    base = table->container->name.name + "_actions";
    actionEnumName = program->refMap->newName(base);

    base = instanceName + "_NoAction";
    noActionName = program->refMap->newName(base);

    keyGenerator = table->container->getKey();
    actionList = table->container->getActionList();

    setTableSize(table);
    setTableKind();
}

void UBPFTable::emitInstance(EBPF::CodeBuilder *builder) {

    cstring keyTypeStr;
    if (keyType != nullptr && keyType->is<IR::Type_Bits>()) {
        auto tb = keyType->to<IR::Type_Bits>();
        auto scalar = new UBPFScalarType(tb);
        keyTypeStr = scalar->getAsString();
    } else {
        keyTypeStr = cstring("struct ") + keyTypeName.c_str();;
    }

    cstring valueTypeStr;
    if (valueType != nullptr && valueType->is<IR::Type_Bits>()) {
        auto tb = valueType->to<IR::Type_Bits>();
        auto scalar = new UBPFScalarType(tb);
        valueTypeStr = scalar->getAsString();
    } else {
        valueTypeStr = cstring("struct ") + valueTypeName.c_str();
    }

    builder->target->emitTableDecl(builder, dataMapName, tableKind,
                                   keyTypeStr,valueTypeStr, size);
}

void UBPFTable::setTableKind()
{
    if (keyGenerator == nullptr) {
        return;
    }
    // set table kind to HASH by default
    EBPF::TableKind tableKind = EBPF::TableHash;

    // If any key field is LPM we will generate an LPM table
    for (auto it : keyGenerator->keyElements) {
        auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
        auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
        if (matchType->name.name == P4::P4CoreLibrary::instance.lpmMatch.name) {
            if (tableKind == EBPF::TableLPMTrie) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "only one LPM field allowed", it->matchType);
                return;
            }
            tableKind = EBPF::TableLPMTrie;
        }
    }
    this->tableKind = tableKind;
}

void UBPFTable::setTableSize(const IR::TableBlock *table) {
    auto properties = table->container->properties->properties;
    this->size = UINT16_MAX;  // Default value 2^16. Next power is too big for ubpf vm.
                              // For instance, 2^17 causes error while loading program.

    auto sz = table->container->getSizeProperty();
    if (sz == nullptr)
        return;

    auto pConstant = sz->to<IR::Constant>();
    if (pConstant->asInt() <= 0) {
        ::error(ErrorType::ERR_INVALID, "negative size", pConstant);
        return;
    }

    this->size = pConstant->asInt();
    if (this->size > UINT16_MAX) {
        ::error(ErrorType::ERR_UNSUPPORTED, "size too large. Using default value (%2%).",
                pConstant, UINT16_MAX);
        return;
    }
}

void UBPFTable::emitKeyType(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("struct %s ", keyTypeName.c_str());
    builder->blockStart();

    EBPF::CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    if (keyGenerator != nullptr) {
        // Use this to order elements by size
        std::multimap<size_t, const IR::KeyElement *> ordered;
        unsigned fieldNumber = 0;
        for (auto c : keyGenerator->keyElements) {
            auto type = program->typeMap->getType(c->expression);
            auto ebpfType = UBPFTypeFactory::instance->create(type);
            cstring fieldName = c->expression->toString().replace('.', '_');
            if (!ebpfType->is<EBPF::IHasWidth>()) {
                ::error("%1%: illegal type %2% for key field", c, type);
                return;
            }
            unsigned width = ebpfType->to<EBPF::IHasWidth>()->widthInBits();
            ordered.emplace(width, c);
            keyTypes.emplace(c, ebpfType);
            keyFieldNames.emplace(c, fieldName);
            fieldNumber++;
        }

        // Emit key in decreasing order size - this way there will be no gaps
        unsigned key_idx = 0;
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
            auto c = it->second;
            if (tableKind == EBPF::TableLPMTrie) {
                builder->emitIndent();
                builder->appendFormat("uint32_t prefix_len%d;", key_idx);
                builder->newline();
            }
            auto ebpfType = ::get(keyTypes, c);
            builder->emitIndent();
            cstring fieldName = ::get(keyFieldNames, c);
            ebpfType->declare(builder, fieldName, false);
            builder->append("; /* ");
            c->expression->apply(commentGen);
            builder->append(" */");
            builder->newline();

            auto mtdecl = program->refMap->getDeclaration(
                    c->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name != P4::P4CoreLibrary::instance.exactMatch.name &&
                matchType->name.name != P4::P4CoreLibrary::instance.lpmMatch.name)
                ::error("Match of type %1% not supported", c->matchType);
            key_idx++;
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void UBPFTable::emitActionArguments(EBPF::CodeBuilder *builder,
                                    const IR::P4Action *action,
                                    cstring name) {
    builder->emitIndent();
    builder->append("struct ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = UBPFTypeFactory::instance->create(p->type);
        type->declare(builder, p->name.name, false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

cstring UBPFTable::generateActionName(const IR::P4Action *action) {
    if (action->getName().originalName == P4::P4CoreLibrary::instance.noAction.name) {
        return this->noActionName;
    } else {
        return EBPF::EBPFObject::externalName(action);
    }
}

void UBPFTable::emitValueType(EBPF::CodeBuilder *builder) {
    // create an enum with tags for all actions
    builder->emitIndent();
    builder->append("enum ");
    builder->append(actionEnumName);
    builder->spc();
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = generateActionName(action);
        builder->emitIndent();
        builder->append(name);
        builder->append(",");
        builder->newline();
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);

    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->appendFormat("struct %s ", valueTypeName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("enum %s action;", actionEnumName.c_str());
    builder->newline();

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = generateActionName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");


    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void UBPFTable::emitTypes(EBPF::CodeBuilder *builder) {
    emitKeyType(builder);
    emitValueType(builder);
}

void UBPFTable::emitKey(EBPF::CodeBuilder *builder, cstring keyName) {
    if (keyGenerator == nullptr)
        return;
    for (auto c : keyGenerator->keyElements) {
        auto ebpfType = ::get(keyTypes, c);
        cstring fieldName = ::get(keyFieldNames, c);
        CHECK_NULL(fieldName);
        bool memcpy = false;
        EBPF::EBPFScalarType *scalar = nullptr;
        unsigned width = 0;
        if (ebpfType->is<EBPF::EBPFScalarType>()) {
            scalar = ebpfType->to<EBPF::EBPFScalarType>();
            width = scalar->implementationWidthInBits();
            memcpy = !EBPF::EBPFScalarType::generatesScalar(width);
        }

        builder->emitIndent();
        if (memcpy) {
            builder->appendFormat("memcpy(&%s.%s, &", keyName.c_str(),
                                  fieldName.c_str());
            codeGen->visit(c->expression);
            builder->appendFormat(", %d)", scalar->bytesRequired());
        } else {
            builder->appendFormat("%s.%s = ", keyName.c_str(),
                                  fieldName.c_str());
            codeGen->visit(c->expression);
        }
        builder->endOfStatement(true);
    }
}

void UBPFTable::emitAction(EBPF::CodeBuilder *builder, cstring valueName) {
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName.c_str());
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        builder->emitIndent();
        cstring name = generateActionName(action);
        builder->appendFormat("case %s: ", name.c_str());
        builder->newline();
        builder->emitIndent();
        builder->blockStart();
        builder->emitIndent();

        UbpfActionTranslationVisitor visitor(valueName, program);
        visitor.setBuilder(builder);
        visitor.copySubstitutions(codeGen);

        action->apply(visitor);
        builder->newline();
        builder->blockEnd(true);
        builder->emitIndent();
        builder->appendLine("break;");
    }

    builder->emitIndent();
    builder->appendFormat("default: return %s",
                          builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);

    builder->blockEnd(true);
}
}  // namespace UBPF



