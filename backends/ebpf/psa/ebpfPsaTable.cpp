/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#include <algorithm>

#include "backends/ebpf/ebpfType.h"
#include "ebpfPsaTable.h"
#include "ebpfPipeline.h"
#include "externs/ebpfPsaTableImplementation.h"

namespace EBPF {

class EBPFTablePsaPropertyVisitor : public Inspector {
 protected:
    EBPFTablePSA* table;

 public:
    explicit EBPFTablePsaPropertyVisitor(EBPFTablePSA* table) : table(table) {}

    // Use these two preorders to print error when property contains something other than name of
    // extern instance. ListExpression is required because without it Expression will take
    // precedence over it and throw error for whole list.
    bool preorder(const IR::ListExpression*) override {
        return true;
    }
    bool preorder(const IR::Expression* expr) override {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: unsupported expression, expected a named instance", expr);
        return false;
    }

    void visitTableProperty(cstring propertyName) {
        auto property = table->table->container->properties->getProperty(propertyName);
        if (property != nullptr)
            property->apply(*this);
    }
};

class EBPFTablePSADirectCounterPropertyVisitor : public EBPFTablePsaPropertyVisitor {
 public:
    explicit EBPFTablePSADirectCounterPropertyVisitor(EBPFTablePSA* table)
        : EBPFTablePsaPropertyVisitor(table) {}

    bool preorder(const IR::PathExpression* pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        if (EBPFObject::getSpecializedTypeName(di) != "DirectCounter") {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: not a DirectCounter, see declaration of %2%", pe, decl);
            return false;
        }

        auto counterName = EBPFObject::externalName(di);
        auto ctr = new EBPFCounterPSA(table->program, di, counterName, table->codeGen);
        table->counters.emplace_back(std::make_pair(counterName, ctr));

        return false;
    }

    void visitTableProperty() {
        EBPFTablePsaPropertyVisitor::visitTableProperty("psa_direct_counter");
    }
};

class EBPFTablePSADirectMeterPropertyVisitor : public EBPFTablePsaPropertyVisitor {
 public:
    explicit EBPFTablePSADirectMeterPropertyVisitor(EBPFTablePSA* table)
        : EBPFTablePsaPropertyVisitor(table) {}

    bool preorder(const IR::PathExpression* pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        if (EBPFObject::getTypeName(di) != "DirectMeter") {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: not a DirectMeter, see declaration of %2%", pe, decl);
            return false;
        }

        auto meterName = EBPFObject::externalName(di);
        auto met = new EBPFMeterPSA(table->program, meterName, di, table->codeGen);
        table->meters.emplace_back(std::make_pair(meterName, met));
        return false;
    }

    void visitTableProperty() {
        EBPFTablePsaPropertyVisitor::visitTableProperty("psa_direct_meter");
    }
};

class EBPFTablePSAImplementationPropertyVisitor : public EBPFTablePsaPropertyVisitor {
 public:
    explicit EBPFTablePSAImplementationPropertyVisitor(EBPFTablePSA* table)
        : EBPFTablePsaPropertyVisitor(table) {}

    // PSA table is allowed to have up to one table implementation. This visitor
    // will iterate over all entries in property, so lets use this and print errors.
    bool preorder(const IR::PathExpression* pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        cstring type = di->type->toString();

        if (table->implementation != nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: Up to one implementation is supported in a table", pe);
            return false;
        }

        if (type == "ActionProfile" || type == "ActionSelector") {
            auto ap = table->program->control->getTable(di->name.name);
            table->implementation = ap->to<EBPFTableImplementationPSA>();
        }

        if (table->implementation != nullptr)
            table->implementation->registerTable(table);
        else
            ::error(ErrorType::ERR_UNKNOWN,
                    "%1%: unknown table implementation %2%", pe, decl);

        return false;
    }

    void visitTableProperty() {
        EBPFTablePsaPropertyVisitor::visitTableProperty("psa_implementation");
    }
};

// =====================ActionTranslationVisitorPSA=============================
ActionTranslationVisitorPSA::ActionTranslationVisitorPSA(const EBPFProgram* program,
                                                         cstring valueName,
                                                         const EBPFTablePSA* table) :
        CodeGenInspector(program->refMap, program->typeMap),
        ActionTranslationVisitor(valueName, program),
        ControlBodyTranslatorPSA(program->to<EBPFPipeline>()->control),
        table(table) {}

bool ActionTranslationVisitorPSA::preorder(const IR::PathExpression* pe) {
    if (isActionParameter(pe)) {
        return ActionTranslationVisitor::preorder(pe);
    }
    return ControlBodyTranslator::preorder(pe);
}

bool ActionTranslationVisitorPSA::isActionParameter(const IR::Expression *expression) const {
    if (auto path = expression->to<IR::PathExpression>())
        return ActionTranslationVisitor::isActionParameter(path);
    else if (auto cast = expression->to<IR::Cast>())
        return isActionParameter(cast->expr);
    else
        return false;
}

void ActionTranslationVisitorPSA::processMethod(const P4::ExternMethod* method) {
    auto declType = method->originalExternType;
    auto decl = method->object;
    BUG_CHECK(decl->is<IR::Declaration_Instance>(),
              "Extern has not been declared: %1%", decl);
    auto di = decl->to<IR::Declaration_Instance>();
    auto instanceName = EBPFObject::externalName(di);

    if (declType->name.name == "DirectCounter") {
        auto ctr = table->getDirectCounter(instanceName);
        if (ctr != nullptr)
            ctr->emitDirectMethodInvocation(builder, method, valueName);
        else
            ::error(ErrorType::ERR_NOT_FOUND,
                    "%1%: Table %2% does not own DirectCounter named %3%",
                    method->expr, table->table->container, instanceName);
    } else if (declType->name.name == "DirectMeter") {
        auto met = table->getMeter(instanceName);
        if (met != nullptr) {
            met->emitDirectExecute(builder, method, valueName);
        } else {
            ::error(ErrorType::ERR_NOT_FOUND,
                    "%1%: Table %2% does not own DirectMeter named %3%",
                    method->expr, table->table->container, instanceName);
        }
    } else {
        ControlBodyTranslatorPSA::processMethod(method);
    }
}

cstring ActionTranslationVisitorPSA::getParamInstanceName(
        const IR::Expression *expression) const {
    if (auto cast = expression->to<IR::Cast>())
        expression = cast->expr;

    return ActionTranslationVisitor::getParamInstanceName(expression);
}

cstring ActionTranslationVisitorPSA::getParamName(const IR::PathExpression *expr) {
    if (isActionParameter(expr)) {
        return getParamInstanceName(expr);
    }

    return ControlBodyTranslatorPSA::getParamName(expr);
}

// =====================EBPFTablePSA=============================
EBPFTablePSA::EBPFTablePSA(const EBPFProgram* program, const IR::TableBlock* table,
                           CodeGenInspector* codeGen) :
                           EBPFTable(program, table, codeGen), implementation(nullptr) {
    auto sizeProperty = table->container->properties->getProperty("size");
    if (keyGenerator == nullptr && sizeProperty != nullptr) {
        ::warning(ErrorType::WARN_IGNORE_PROPERTY,
                  "%1%: property ignored because table does not have a key", sizeProperty);
    }

    if (keyFieldNames.empty() && size != 1) {
        if (sizeProperty != nullptr) {
            ::warning(ErrorType::WARN_IGNORE,
                      "%1%: only one entry allowed with empty key or selector-only key",
                      sizeProperty);
        }
        this->size = 1;
    }

    initDirectCounters();
    initDirectMeters();
    initImplementation();
}

EBPFTablePSA::EBPFTablePSA(const EBPFProgram* program, CodeGenInspector* codeGen, cstring name) :
                           EBPFTable(program, codeGen, name), implementation(nullptr) {}

void EBPFTablePSA::initDirectCounters() {
    EBPFTablePSADirectCounterPropertyVisitor visitor(this);
    visitor.visitTableProperty();
}

void EBPFTablePSA::initDirectMeters() {
    EBPFTablePSADirectMeterPropertyVisitor visitor(this);
    visitor.visitTableProperty();
}

void EBPFTablePSA::initImplementation() {
    EBPFTablePSAImplementationPropertyVisitor visitor(this);
    visitor.visitTableProperty();

    bool hasActionSelector = (implementation != nullptr &&
            implementation->is<EBPFActionSelectorPSA>());

    // check if we have also selector key
    const IR::KeyElement * selectorKey = nullptr;
    if (keyGenerator != nullptr) {
        for (auto k : keyGenerator->keyElements) {
            auto mkdecl = program->refMap->getDeclaration(k->matchType->path, true);
            auto matchType = mkdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name == "selector") {
                selectorKey = k;
                break;
            }
        }
    }

    if (hasActionSelector && selectorKey == nullptr) {
        ::error(ErrorType::ERR_NOT_FOUND,
                "%1%: ActionSelector provided but there is no selector key",
                table->container);
    }
    if (!hasActionSelector && selectorKey != nullptr) {
        ::error(ErrorType::ERR_NOT_FOUND,
                "%1%: implementation not found, ActionSelector is required",
                selectorKey->matchType);
    }
    auto emptyGroupAction = table->container->properties->getProperty("psa_empty_group_action");
    if (!hasActionSelector && emptyGroupAction != nullptr) {
        ::warning(ErrorType::WARN_UNUSED,
                  "%1%: unused property (ActionSelector not provided)",
                  emptyGroupAction);
    }
}

ActionTranslationVisitor* EBPFTablePSA::createActionTranslationVisitor(
        cstring valueName, const EBPFProgram* program) const {
    return new ActionTranslationVisitorPSA(program->to<EBPFPipeline>(), valueName, this);
}

void EBPFTablePSA::emitValueStructStructure(CodeBuilder* builder) {
    if (implementation != nullptr) {
        if (isTernaryTable()) {
            builder->emitIndent();
            builder->append("__u32 priority;");
            builder->newline();
        }
        implementation->emitReferenceEntry(builder);
    } else {
        EBPFTable::emitValueStructStructure(builder);
    }
}

void EBPFTablePSA::emitInstance(CodeBuilder *builder) {
    if (isTernaryTable()) {
        emitTernaryInstance(builder);
        if (hasConstEntries()) {
            auto entries = getConstEntriesGroupedByPrefix();
            // A number of tuples is equal to number of unique prefixes
            int nrOfTuples = entries.size();
            for (int i = 0; i < nrOfTuples; i++) {
                builder->target->emitTableDecl(builder,
                                               instanceName + "_tuple_" + std::to_string(i),
                                               TableHash,"struct " + keyTypeName,
                                               "struct " + valueTypeName, size);
            }
        }
    } else {
        TableKind kind = isLPMTable() ? TableLPMTrie : TableHash;
        builder->target->emitTableDecl(builder, instanceName, kind,
                      cstring("struct ") + keyTypeName,
                      cstring("struct ") + valueTypeName, size);
    }

    if (implementation == nullptr) {
        // Default action is up to implementation, define it when no implementation provided
        builder->target->emitTableDecl(builder, defaultActionMapName, TableArray,
                      program->arrayIndexType,
                      cstring("struct ") + valueTypeName, 1);
    }
}

void EBPFTablePSA::emitTypes(CodeBuilder* builder) {
    EBPFTable::emitTypes(builder);
    // TODO: placeholder for handling PSA-specific types
}

/**
 * Order of emitting counters and meters affects generated layout of BPF map value.
 * Do not change this order!
 */
void EBPFTablePSA::emitDirectValueTypes(CodeBuilder* builder) {
    for (auto ctr : counters) {
        ctr.second->emitValueType(builder);
    }
    for (auto met : meters) {
        met.second->emitValueType(builder);
    }
    if (!meters.empty()) {
        meters.begin()->second->emitSpinLockField(builder);
    }
}

void EBPFTablePSA::emitAction(CodeBuilder* builder, cstring valueName, cstring actionRunVariable) {
    if (implementation != nullptr)
        implementation->applyImplementation(builder, valueName, actionRunVariable);
    else
        EBPFTable::emitAction(builder, valueName, actionRunVariable);
}

void EBPFTablePSA::emitInitializer(CodeBuilder *builder) {
    // Do not emit initializer when table implementation is provided, because it is not supported.
    // Error for such case is printed when adding implementation to a table.
    if (implementation == nullptr) {
        this->emitDefaultActionInitializer(builder);
        this->emitConstEntriesInitializer(builder);
    }
}

void EBPFTablePSA::emitConstEntriesInitializer(CodeBuilder *builder) {
    if (isTernaryTable()) {
        emitTernaryConstEntriesInitializer(builder);
        return;
    }
    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);
    const IR::EntriesList* entries = table->container->getEntries();
    if (entries != nullptr) {
        for (auto entry : entries->entries) {
            auto keyName = program->refMap->newName("key");
            auto valueName = program->refMap->newName("value");
            // construct key
            builder->emitIndent();
            builder->appendFormat("struct %s %s = {}", this->keyTypeName.c_str(), keyName.c_str());
            builder->endOfStatement(true);
            for (size_t index = 0; index < keyGenerator->keyElements.size(); index++) {
                auto keyElement = keyGenerator->keyElements[index];
                cstring fieldName = get(keyFieldNames, keyElement);
                CHECK_NULL(fieldName);
                builder->emitIndent();
                builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
                auto mtdecl = program->refMap->getDeclaration(keyElement->matchType->path, true);
                auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchType->name.name == P4::P4CoreLibrary::instance.lpmMatch.name) {
                    auto expr = entry->keys->components[index];

                    auto ebpfType = ::get(keyTypes, keyElement);
                    unsigned width = 0;
                    cstring swap;
                    if (ebpfType->is<EBPFScalarType>()) {
                        auto scalar = ebpfType->to<EBPFScalarType>();
                        width = scalar->implementationWidthInBits();

                        if (width <= 8) {
                            swap = "";  // single byte, nothing to swap
                        } else if (width <= 16) {
                            swap = "bpf_htons";
                        } else if (width <= 32) {
                            swap = "bpf_htonl";
                        } else if (width <= 64) {
                            swap = "bpf_htonll";
                        } else {
                            // TODO: handle width > 64 bits
                            ::error(ErrorType::ERR_UNSUPPORTED,
                                    "%1%: fields wider than 64 bits are not supported yet",
                                    fieldName);
                        }
                    }
                    builder->appendFormat("%s(", swap);
                    if (auto km = expr->to<IR::Mask>()) {
                        km->left->apply(cg);
                    } else {
                        expr->apply(cg);
                    }
                    builder->append(")");
                    builder->endOfStatement(true);
                    builder->emitIndent();
                    builder->appendFormat("%s.%s = ", keyName.c_str(), prefixFieldName.c_str());
                    unsigned prefixLen = 32;
                    if (auto km = expr->to<IR::Mask>()) {
                        auto trailing_zeros = [width](const big_int& n) -> int {
                            return (n == 0) ? width : boost::multiprecision::lsb(n); };
                        auto count_ones = [](const big_int& n) -> unsigned {
                            return bitcount(n); };
                        auto mask = km->right->to<IR::Constant>()->value;
                        auto len = trailing_zeros(mask);
                        if (len + count_ones(mask) != width) {  // any remaining 0s in the prefix?
                            ::error(ErrorType::ERR_INVALID,
                                    "%1% invalid mask for LPM key", keyElement);
                            return;
                        }
                        prefixLen = width - len;
                    }
                    builder->append(prefixLen);
                    builder->endOfStatement(true);

                } else if (matchType->name.name == P4::P4CoreLibrary::instance.exactMatch.name) {
                    entry->keys->components[index]->apply(cg);
                    builder->endOfStatement(true);
                }
            }

            // construct value
            auto *mce = entry->action->to<IR::MethodCallExpression>();
            emitTableValue(builder, mce, valueName.c_str());

            // emit update
            auto ret = program->refMap->newName("ret");
            builder->emitIndent();
            builder->appendFormat("int %s = ", ret.c_str());
            builder->target->emitTableUpdate(builder, instanceName,
                                             keyName.c_str(), valueName.c_str());
            builder->newline();

            emitMapUpdateTraceMsg(builder, instanceName, ret);
        }
    }
}

void EBPFTablePSA::emitDefaultActionInitializer(CodeBuilder *builder) {
    const IR::P4Table* t = table->container;
    const IR::Expression* defaultAction = t->getDefaultAction();
    auto actionName = getActionNameExpression(defaultAction);
    auto mce = defaultAction->to<IR::MethodCallExpression>();
    CHECK_NULL(actionName);
    CHECK_NULL(mce);
    if (actionName->path->name.originalName != P4::P4CoreLibrary::instance.noAction.name) {
        auto value = program->refMap->newName("value");
        emitTableValue(builder, mce, value.c_str());
        auto ret = program->refMap->newName("ret");
        builder->emitIndent();
        builder->appendFormat("int %s = ", ret.c_str());
        builder->target->emitTableUpdate(builder, defaultActionMapName,
                                         program->zeroKey.c_str(), value.c_str());
        builder->newline();

        emitMapUpdateTraceMsg(builder, defaultActionMapName, ret);
    }
}

void EBPFTablePSA::emitMapUpdateTraceMsg(CodeBuilder *builder, cstring mapName,
                                         cstring returnCode) const {
    if (!program->options.emitTraceMessages) {
        return;
    }
    builder->emitIndent();
    builder->appendFormat("if (%s) ", returnCode.c_str());
    builder->blockStart();
    cstring msgStr = Util::printf_format("Map initializer: Error while map (%s) update, code: %s",
                                         mapName, "%d");
    builder->target->emitTraceMessage(builder,
                                      msgStr, 1, returnCode.c_str());

    builder->blockEnd(false);
    builder->append(" else ");

    builder->blockStart();
    msgStr = Util::printf_format("Map initializer: Map (%s) update succeed",
                                 mapName, returnCode.c_str());
    builder->target->emitTraceMessage(builder,
                                      msgStr);
    builder->blockEnd(true);
}

const IR::PathExpression* EBPFTablePSA::getActionNameExpression(const IR::Expression* expr) const {
    BUG_CHECK(expr->is<IR::MethodCallExpression>(),
            "%1%: expected an action call", expr);
    auto mce = expr->to<IR::MethodCallExpression>();
    BUG_CHECK(mce->method->is<IR::PathExpression>(),
            "%1%: expected IR::PathExpression type", mce->method);
    return mce->method->to<IR::PathExpression>();
}

void EBPFTablePSA::emitTableValue(CodeBuilder* builder, const IR::MethodCallExpression* actionMce,
                                  cstring valueName) {
    auto mi = P4::MethodInstance::resolve(actionMce, program->refMap, program->typeMap);
    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mi);
    auto action = ac->action;

    cstring actionName = EBPFObject::externalName(action);

    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    builder->emitIndent();
    builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), valueName.c_str());
    builder->blockStart();
    builder->emitIndent();
    cstring fullActionName = p4ActionToActionIDName(action);
    builder->appendFormat(".action = %s,", fullActionName);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".u = {.%s = {", actionName.c_str());
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        arg->apply(cg);
        builder->append(",");
    }
    builder->append("}},\n");
    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFTablePSA::emitLookupDefault(CodeBuilder* builder, cstring key, cstring value,
                                     cstring actionRunVariable) {
    if (implementation != nullptr) {
        builder->appendLine("/* table with implementation has default action "
                            "implicitly set to NoAction, so we can skip execution of it */");
        builder->target->emitTraceMessage(builder,
                                          "Control: skipping default action due to implementation");

        if (!actionRunVariable.isNullOrEmpty()) {
            builder->emitIndent();
            builder->appendFormat("%s = 0", actionRunVariable.c_str());  // set to NoAction
            builder->endOfStatement(true);
        }
    } else {
        EBPFTable::emitLookupDefault(builder, key, value, actionRunVariable);
    }
}

bool EBPFTablePSA::dropOnNoMatchingEntryFound() const {
    if (implementation != nullptr)
        return false;
    return EBPFTable::dropOnNoMatchingEntryFound();
}

void EBPFTablePSA::emitTernaryConstEntriesInitializer(CodeBuilder *builder) {
    std::vector<std::vector<const IR::Entry*>> entriesGroupedByPrefix =
            getConstEntriesGroupedByPrefix();
    if (entriesGroupedByPrefix.empty())
        return;

    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);
    std::vector<cstring> keyMasksNames;
    cstring uniquePrefix = instanceName;
    int tuple_id = 0;  // We have preallocated tuple maps with ids starting from 0

    // emit key head mask
    cstring headName = program->refMap->newName("key_mask");
    builder->emitIndent();
    builder->appendFormat("struct %s_mask %s = {0}", keyTypeName, headName);
    builder->endOfStatement(true);

    // emit key masks
    emitKeyMasks(builder, entriesGroupedByPrefix, keyMasksNames);

    builder->newline();

    // add head
    cstring valueMask = program->refMap->newName("value_mask");
    cstring nextMask = keyMasksNames[0];
    int noTupleId = -1;
    emitValueMask(builder, valueMask, nextMask, noTupleId);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("%s(0, 0, &%s, &%s, &%s, &%s, NULL, NULL)",
                          addPrefixFunctionName, tuplesMapName,
                          prefixesMapName, headName, valueMask);
    builder->endOfStatement(true);
    builder->newline();

    // emit values + updates
    for (size_t i = 0; i < entriesGroupedByPrefix.size(); i++) {
        auto samePrefixEntries = entriesGroupedByPrefix[i];
        valueMask = program->refMap->newName("value_mask");
        std::vector<cstring> keyNames;
        std::vector<cstring> valueNames;
        cstring keysArray = program->refMap->newName("keys");
        cstring valuesArray = program->refMap->newName("values");
        cstring keyMaskVarName = keyMasksNames[i];

        if (entriesGroupedByPrefix.size() > i + 1) {
            nextMask = keyMasksNames[i + 1];
        }
        emitValueMask(builder, valueMask, nextMask, tuple_id);
        builder->newline();
        emitKeysAndValues(builder, samePrefixEntries, keyNames, valueNames);

        // construct keys array
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("void *%s[] = {", keysArray);
        for (auto keyName : keyNames)
            builder->appendFormat("&%s,", keyName);
        builder->append("}");
        builder->endOfStatement(true);

        // construct values array
        builder->emitIndent();
        builder->appendFormat("void *%s[] = {", valuesArray);
        for (auto valueName : valueNames)
            builder->appendFormat("&%s,", valueName);
        builder->append("}");
        builder->endOfStatement(true);

        builder->newline();
        builder->emitIndent();
        builder->appendFormat("%s(%s, %s, &%s, &%s, &%s, &%s, %s, %s)",
                              addPrefixFunctionName,
                              cstring::to_cstring(samePrefixEntries.size()),
                              cstring::to_cstring(tuple_id),
                              tuplesMapName,
                              prefixesMapName,
                              keyMaskVarName,
                              valueMask,
                              keysArray,
                              valuesArray);
        builder->endOfStatement(true);

        tuple_id++;
    }
}

void EBPFTablePSA::emitKeysAndValues(CodeBuilder *builder,
                                     std::vector<const IR::Entry *> &samePrefixEntries,
                                     std::vector<cstring> &keyNames,
                                     std::vector<cstring> &valueNames) {
    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    for (auto entry : samePrefixEntries) {
        cstring keyName = program->refMap->newName("key");
        cstring valueName = program->refMap->newName("value");
        keyNames.push_back(keyName);
        valueNames.push_back(valueName);
        // construct key
        builder->emitIndent();
        builder->appendFormat("struct %s %s = {}", keyTypeName.c_str(), keyName.c_str());
        builder->endOfStatement(true);
        for (size_t k = 0; k < keyGenerator->keyElements.size(); k++) {
            auto keyElement = keyGenerator->keyElements[k];
            cstring fieldName = get(keyFieldNames, keyElement);
            CHECK_NULL(fieldName);
            builder->emitIndent();
            builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
            auto mtdecl = program->refMap->getDeclaration(keyElement->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            auto expr = entry->keys->components[k];
            auto ebpfType = get(keyTypes, keyElement);
            if (auto km = expr->to<IR::Mask>()) {
                km->left->apply(cg);
                builder->append(" & ");
                km->right->apply(cg);
            } else {
                expr->apply(cg);
                builder->append(" & ");
                emitMaskForExactMatch(builder, fieldName, ebpfType);
            }
            builder->endOfStatement(true);
        }

        // construct value
        auto *mce = entry->action->to<IR::MethodCallExpression>();
        emitTableValue(builder, mce, valueName.c_str());
    }
}

void EBPFTablePSA::emitKeyMasks(CodeBuilder *builder,
                                std::vector<std::vector<const IR::Entry *>> &entriesGrpedByPrefix,
                                std::vector<cstring> &keyMasksNames) {
    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    for (auto samePrefixEntries : entriesGrpedByPrefix) {
        auto firstEntry = samePrefixEntries.front();
        cstring keyFieldName = program->refMap->newName("key_mask");
        keyMasksNames.push_back(keyFieldName);

        builder->emitIndent();
        builder->appendFormat("struct %s_mask %s = {0}", keyTypeName, keyFieldName);
        builder->endOfStatement(true);

        builder->emitIndent();
        cstring keyFieldNamePtr = program->refMap->newName(keyFieldName + "_ptr");
        builder->appendFormat("char *%s = &%s.mask", keyFieldNamePtr, keyFieldName);
        builder->endOfStatement(true);

        for (size_t i = 0; i < keyGenerator->keyElements.size(); i++) {
            auto keyElement = keyGenerator->keyElements[i];
            auto expr = firstEntry->keys->components[i];
            cstring fieldName = program->refMap->newName("field");
            auto ebpfType = get(keyTypes, keyElement);
            builder->emitIndent();
            ebpfType->declare(builder, fieldName, false);
            builder->append(" = ");
            if (auto mask = expr->to<IR::Mask>()) {
                mask->right->apply(cg);
                builder->endOfStatement(true);
            } else {
                // MidEnd transforms 0xffff... masks into exact match
                // So we receive there a Constant same as exact match
                // So we have to create 0xffff... mask on our own
                emitMaskForExactMatch(builder, fieldName, ebpfType);
            }
            builder->emitIndent();
            builder->appendFormat("__builtin_memcpy(%s, &%s, sizeof(%s))",
                                  keyFieldNamePtr, fieldName, fieldName);
            builder->endOfStatement(true);
            builder->appendFormat("%s = %s + sizeof(%s)", keyFieldNamePtr,
                                  keyFieldNamePtr, fieldName);
            builder->endOfStatement(true);
        }
    }
}

void EBPFTablePSA::emitMaskForExactMatch(CodeBuilder *builder,
                                         cstring &fieldName,
                                         EBPFType *ebpfType) const {
    unsigned width = 0;
    if (auto hasWidth = ebpfType->to<IHasWidth>()) {
        width = hasWidth->widthInBits();
        if (width <= 8) {
            width = 8;
        } else if (width <= 16) {
            width = 16;
        } else if (width <= 32) {
            width = 32;
        } else if (width <= 64) {
            width = 64;
        } else {
            // TODO: handle width > 64 bits
            error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: fields wider than 64 bits are not supported yet",
                    fieldName);
        }
    } else {
        BUG("Cannot assess field bit width");
    }
    builder->append("0x");
    for (int j = 0; j < width / 8; j++)
        builder->append("ff");
    builder->endOfStatement(true);
}

void EBPFTablePSA::emitValueMask(CodeBuilder *builder, const cstring valueMask,
                                        const cstring nextMask, int tupleId) const {
    builder->emitIndent();
    builder->appendFormat("struct %s_mask %s = {0}", valueTypeName, valueMask);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s.tuple_id = %s", valueMask, cstring::to_cstring(tupleId));
    builder->endOfStatement(true);
    builder->emitIndent();
    if (nextMask.isNullOrEmpty()) {
        builder->appendFormat("%s.has_next = 0", valueMask);
        builder->endOfStatement(true);
    } else {
        builder->appendFormat("%s.next_tuple_mask = %s", valueMask, nextMask);
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat("%s.has_next = 1", valueMask);
        builder->endOfStatement(true);
    }
}

/**
 * This method groups entries with the same prefix into separate lists.
 * For example four entries which have two different prefixes
 * will give as a result a vector of two vectors (each with two entries).
 * @return a vector of vectors with const entries that have the same prefix
 */
std::vector<std::vector<const IR::Entry*>> EBPFTablePSA::getConstEntriesGroupedByPrefix() {
    std::vector<std::vector<const IR::Entry*>> entriesGroupedByPrefix;
    const IR::EntriesList* entries = table->container->getEntries();

    if (!entries)
        return entriesGroupedByPrefix;

    for (int i = 0; i < (int)entries->entries.size(); i++) {
        auto mainEntr = entries->entries[i];
        if (!entriesGroupedByPrefix.empty()) {
            auto last = entriesGroupedByPrefix.back();
            auto it = std::find(last.begin(), last.end(), mainEntr);
            if (it != last.end()) {
                // If this entry was added in a previous iteration
                continue;
            }
        }
        std::vector<const IR::Entry*> samePrefEntries;
        samePrefEntries.push_back(mainEntr);
        for (int j = i; j < (int)entries->entries.size(); j++) {
            auto refEntr = entries->entries[j];
            if (i != j) {
                bool isTheSamePrefix = true;
                for (size_t k = 0; k < mainEntr->keys->components.size(); k++) {
                    auto k1 = mainEntr->keys->components[k];
                    auto k2 = refEntr->keys->components[k];
                    if (auto k1Mask = k1->to<IR::Mask>()) {
                        if (auto k2Mask = k2->to<IR::Mask>()) {
                            auto val1 = k1Mask->right->to<IR::Constant>();
                            auto val2 = k2Mask->right->to<IR::Constant>();
                            if (val1->value != val2->value) {
                                isTheSamePrefix = false;
                                break;
                            }
                        }
                    }
                }
                if (isTheSamePrefix) {
                    samePrefEntries.push_back(refEntr);
                }
            }
        }
        entriesGroupedByPrefix.push_back(samePrefEntries);
    }

    return entriesGroupedByPrefix;
}

bool EBPFTablePSA::hasConstEntries() {
    const IR::EntriesList* entries = table->container->getEntries();
    return entries && entries->size() > 0;
}

cstring EBPFTablePSA::addPrefixFunc(bool trace) {
    cstring addPrefixFunc =
            "static __always_inline\n"
            "void add_prefix_and_entries(__u32 nr_entries,\n"
            "            __u32 tuple_id,\n"
            "            void *tuples_map,\n"
            "            void *prefixes_map,\n"
            "            void *key_mask,\n"
            "            void *value_mask,\n"
            "            void *keysPtrs[],\n"
            "            void *valuesPtrs[]) {\n"
            "    int ret = bpf_map_update_elem(prefixes_map, key_mask, value_mask, BPF_ANY);\n"
            "    if (ret) {\n"
            "%trace_msg_prefix_map_fail%"
            "        return;\n"
            "    }\n"
            "    if (nr_entries == 0) {\n"
            "        return;\n"
            "    }\n"
            "    struct bpf_elf_map *tuple = bpf_map_lookup_elem(tuples_map, &tuple_id);\n"
            "    if (tuple) {\n"
            "        for (__u32 i = 0; i < nr_entries; i++) {\n"
            "            int ret = bpf_map_update_elem(tuple, keysPtrs[i], valuesPtrs[i], "
            "BPF_ANY);\n"
            "            if (ret) {\n"
            "%trace_msg_tuple_update_fail%"
            "                return;\n"
            "            } else {\n"
            "%trace_msg_tuple_update_success%"
            "            }\n"
            "        }\n"
            "    } else {\n"
            "%trace_msg_tuple_not_found%"
            "        return;\n"
            "    }\n"
            "}";

    if (trace) {
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_prefix_map_fail%",
                "        bpf_trace_message(\"Prefixes map update failed\\n\");\n");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_update_fail%",
                "                bpf_trace_message(\"Tuple map update failed\\n\");\n");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_update_success%",
                "                bpf_trace_message(\"Tuple map update succeed\\n\");\n");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_not_found%",
                "        bpf_trace_message(\"Tuple not found\\n\");\n");
    } else {
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_prefix_map_fail%",
                "");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_update_fail%",
                "");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_update_success%",
                "");
        addPrefixFunc = addPrefixFunc.replace(
                "%trace_msg_tuple_not_found%",
                "");
    }

    return addPrefixFunc;
}

}  // namespace EBPF
