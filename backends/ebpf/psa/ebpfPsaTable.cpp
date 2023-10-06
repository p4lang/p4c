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
#include "ebpfPsaTable.h"

#include <algorithm>

#include "backends/ebpf/ebpfType.h"
#include "ebpfPipeline.h"
#include "externs/ebpfPsaTableImplementation.h"

namespace EBPF {

class EBPFTablePsaPropertyVisitor : public Inspector {
 protected:
    EBPFTablePSA *table;

 public:
    explicit EBPFTablePsaPropertyVisitor(EBPFTablePSA *table) : table(table) {}

    // Use these two preorders to print error when property contains something other than name of
    // extern instance. ListExpression is required because without it Expression will take
    // precedence over it and throw error for whole list.
    bool preorder(const IR::ListExpression *) override { return true; }
    bool preorder(const IR::Expression *expr) override {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: unsupported expression, expected a named instance", expr);
        return false;
    }

    void visitTableProperty(cstring propertyName) {
        auto property = table->table->container->properties->getProperty(propertyName);
        if (property != nullptr) property->apply(*this);
    }
};

class EBPFTablePSADirectCounterPropertyVisitor : public EBPFTablePsaPropertyVisitor {
 public:
    explicit EBPFTablePSADirectCounterPropertyVisitor(EBPFTablePSA *table)
        : EBPFTablePsaPropertyVisitor(table) {}

    bool preorder(const IR::PathExpression *pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        if (EBPFObject::getSpecializedTypeName(di) != "DirectCounter") {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: not a DirectCounter, see declaration of %2%",
                    pe, decl);
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
    explicit EBPFTablePSADirectMeterPropertyVisitor(EBPFTablePSA *table)
        : EBPFTablePsaPropertyVisitor(table) {}

    bool preorder(const IR::PathExpression *pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        if (EBPFObject::getTypeName(di) != "DirectMeter") {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: not a DirectMeter, see declaration of %2%", pe,
                    decl);
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
    explicit EBPFTablePSAImplementationPropertyVisitor(EBPFTablePSA *table)
        : EBPFTablePsaPropertyVisitor(table) {}

    // PSA table is allowed to have up to one table implementation. This visitor
    // will iterate over all entries in property, so lets use this and print errors.
    bool preorder(const IR::PathExpression *pe) override {
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
            ::error(ErrorType::ERR_UNKNOWN, "%1%: unknown table implementation %2%", pe, decl);

        return false;
    }

    void visitTableProperty() {
        EBPFTablePsaPropertyVisitor::visitTableProperty("psa_implementation");
    }
};

// Generator for table key/value initializer value (const entries). Can't be used during table
// lookup because this inspector expects only constant values as initializer.
class EBPFTablePSAInitializerCodeGen : public CodeGenInspector {
 protected:
    unsigned currentKeyEntryIndex = 0;
    const IR::Entry *currentEntry = nullptr;
    const EBPFTablePSA *table = nullptr;
    bool tableHasTernaryMatch = false;

 public:
    EBPFTablePSAInitializerCodeGen(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                   const EBPFTablePSA *table = nullptr)
        : CodeGenInspector(refMap, typeMap), table(table) {
        if (table) tableHasTernaryMatch = table->isTernaryTable();
    }

    void generateKeyInitializer(const IR::Entry *entry) {
        currentEntry = entry;
        // entry->keys is a IR::ListExpression, it lacks information about table key,
        // we have to visit table->keyGenerator instead.
        table->keyGenerator->apply(*this);
    }

    void generateValueInitializer(const IR::Expression *expr) {
        currentEntry = nullptr;
        expr->apply(*this);
    }

    bool preorder(const IR::Mask *expr) override {
        unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, expr->left);

        if (EBPFScalarType::generatesScalar(width)) {
            visit(expr->left);
            builder->append(" & ");
            visit(expr->right);
        } else {
            auto lc = expr->left->to<IR::Constant>();
            auto rc = expr->right->to<IR::Constant>();
            BUG_CHECK(lc != nullptr, "%1%: expected a constant value", expr->left);
            BUG_CHECK(rc != nullptr, "%1%: expected a constant value", expr->right);
            cstring value = EBPFInitializerUtils::genHexStr(lc->value, width, expr->left);
            cstring mask = EBPFInitializerUtils::genHexStr(rc->value, width, expr->right);
            builder->append("{ ");
            for (size_t i = 0; i < value.size() / 2; ++i)
                builder->appendFormat("(0x%s & 0x%s), ", value.substr(2 * i, 2),
                                      mask.substr(2 * i, 2));
            builder->append("}");
        }

        return false;
    }

    // {pre,post}orders for table key initializer
    bool preorder(const IR::Key *) override {
        BUG_CHECK(table->keyGenerator->keyElements.size() == currentEntry->keys->size(),
                  "Entry key size does not match table key size");
        currentKeyEntryIndex = 0;
        builder->blockStart();
        return true;
    }
    bool preorder(const IR::KeyElement *key) override {
        cstring fieldName = ::get(table->keyFieldNames, key);
        cstring matchType = key->matchType->path->name.name;
        auto expr = currentEntry->keys->components[currentKeyEntryIndex];
        unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, key->expression);
        bool isLPMMatch = matchType == P4::P4CoreLibrary::instance().lpmMatch.name;
        bool genPrefixLen = !tableHasTernaryMatch && isLPMMatch;
        bool doSwapBytes = genPrefixLen && EBPFScalarType::generatesScalar(width);

        builder->emitIndent();
        builder->appendFormat(".%s = ", fieldName.c_str());
        if (doSwapBytes) {
            if (width <= 8) {
                ;  // single byte, nothing to swap
            } else if (width <= 16) {
                builder->append("bpf_htons");
            } else if (width <= 32) {
                builder->append("bpf_htonl");
            } else if (width <= 64) {
                builder->append("bpf_htonll");
            }
            builder->append("(");
        }
        visit(expr);
        if (doSwapBytes) builder->append(")");
        builder->appendLine(",");

        if (genPrefixLen) {
            unsigned prefixLen = width;
            if (auto km = expr->to<IR::Mask>()) {
                auto trailing_zeros = [width](const big_int &n) -> unsigned {
                    return (n == 0) ? width : boost::multiprecision::lsb(n);
                };
                auto count_ones = [](const big_int &n) -> unsigned { return bitcount(n); };
                auto mask = km->right->to<IR::Constant>()->value;
                auto len = trailing_zeros(mask);
                if (len + count_ones(mask) != width) {  // any remaining 0s in the prefix?
                    ::error(ErrorType::ERR_INVALID, "%1% invalid mask for LPM key", key);
                    return false;
                }
                prefixLen = width - len;
            }

            builder->emitIndent();
            builder->appendFormat(".%s = 8*offsetof(struct %s, %s)-32 + %u",
                                  table->prefixFieldName.c_str(), table->keyTypeName.c_str(),
                                  fieldName.c_str(), prefixLen);
            builder->appendLine(",");
        }

        ++currentKeyEntryIndex;
        return false;
    }
    void postorder(const IR::Key *) override { builder->blockEnd(false); }

    // preorder for value table value initializer
    bool preorder(const IR::MethodCallExpression *mce) override {
        auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
        auto ac = mi->to<P4::ActionCall>();
        BUG_CHECK(ac != nullptr, "%1%: expected an action call", mi);
        cstring actionName = EBPFObject::externalName(ac->action);
        cstring fullActionName = table->p4ActionToActionIDName(ac->action);

        builder->blockStart();
        builder->emitIndent();
        builder->appendFormat(".action = %s,", fullActionName);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat(".u = {.%s = {", actionName.c_str());
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            visit(mi->substitution.lookup(p));
            builder->append(", ");
        }
        builder->appendLine("}}");

        builder->blockEnd(false);
        return false;
    }

    bool preorder(const IR::PathExpression *p) override { return notSupported(p); }
};

// Generate mask for whole table key
class EBPFTablePSATernaryTableMaskGenerator : public Inspector {
 protected:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    // Mask generation is done using string concatenation,
    // so use std::string as it behave better in this case than cstring.
    std::string mask;

 public:
    EBPFTablePSATernaryTableMaskGenerator(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

    cstring getMaskStr(const IR::Entry *entry) {
        mask.clear();
        entry->keys->apply(*this);
        return mask;
    }

    bool preorder(const IR::Constant *expr) override {
        // exact match, set all bits as 'care'
        unsigned bytes = ROUNDUP(EBPFInitializerUtils::ebpfTypeWidth(typeMap, expr), 8);
        for (unsigned i = 0; i < bytes; ++i) mask += "ff";
        return false;
    }
    bool preorder(const IR::Mask *expr) override {
        // Available value and mask, so use only this mask
        BUG_CHECK(expr->right->is<IR::Constant>(), "%1%: Expected a constant value", expr->right);
        auto &value = expr->right->to<IR::Constant>()->value;
        unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, expr->right);
        mask += EBPFInitializerUtils::genHexStr(value, width, expr->right);
        return false;
    }
};

// Build mask initializer for a single table key entry
class EBPFTablePSATernaryKeyMaskGenerator : public EBPFTablePSAInitializerCodeGen {
 public:
    EBPFTablePSATernaryKeyMaskGenerator(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : EBPFTablePSAInitializerCodeGen(refMap, typeMap, nullptr) {}

    bool preorder(const IR::Constant *expr) override {
        // MidEnd transforms 0xffff... masks into exact match
        // So we receive there a Constant same as exact match
        // So we have to create 0xffff... mask on our own
        unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, expr);
        unsigned bytes = ROUNDUP(width, 8);

        if (EBPFScalarType::generatesScalar(width)) {
            builder->append("0x");
            for (unsigned i = 0; i < bytes; ++i) builder->append("ff");
        } else {
            builder->append("{ ");
            for (size_t i = 0; i < bytes; ++i) builder->append("0xff, ");
            builder->append("}");
        }

        return false;
    }

    bool preorder(const IR::Mask *expr) override {
        // Mask value is our value which we want to generate
        BUG_CHECK(expr->right->is<IR::Constant>(), "%1%: expected constant value", expr->right);
        CodeGenInspector::preorder(expr->right->to<IR::Constant>());
        return false;
    }
};

// =====================ActionTranslationVisitorPSA=============================
ActionTranslationVisitorPSA::ActionTranslationVisitorPSA(const EBPFProgram *program,
                                                         cstring valueName,
                                                         const EBPFTablePSA *table)
    : CodeGenInspector(program->refMap, program->typeMap),
      ActionTranslationVisitor(valueName, program),
      ControlBodyTranslatorPSA(program->to<EBPFPipeline>()->control),
      table(table) {}

bool ActionTranslationVisitorPSA::preorder(const IR::PathExpression *pe) {
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

void ActionTranslationVisitorPSA::processMethod(const P4::ExternMethod *method) {
    auto declType = method->originalExternType;
    auto decl = method->object;
    BUG_CHECK(decl->is<IR::Declaration_Instance>(), "Extern has not been declared: %1%", decl);
    auto di = decl->to<IR::Declaration_Instance>();
    auto instanceName = EBPFObject::externalName(di);

    if (declType->name.name == "DirectCounter") {
        auto ctr = table->getDirectCounter(instanceName);
        if (ctr != nullptr)
            ctr->emitDirectMethodInvocation(builder, method, valueName);
        else
            ::error(ErrorType::ERR_NOT_FOUND, "%1%: Table %2% does not own DirectCounter named %3%",
                    method->expr, table->table->container, instanceName);
    } else if (declType->name.name == "DirectMeter") {
        auto met = table->getMeter(instanceName);
        if (met != nullptr) {
            met->emitDirectExecute(builder, method, valueName);
        } else {
            ::error(ErrorType::ERR_NOT_FOUND, "%1%: Table %2% does not own DirectMeter named %3%",
                    method->expr, table->table->container, instanceName);
        }
    } else {
        ControlBodyTranslatorPSA::processMethod(method);
    }
}

cstring ActionTranslationVisitorPSA::getParamInstanceName(const IR::Expression *expression) const {
    if (auto cast = expression->to<IR::Cast>()) expression = cast->expr;

    return ActionTranslationVisitor::getParamInstanceName(expression);
}

cstring ActionTranslationVisitorPSA::getParamName(const IR::PathExpression *expr) {
    if (isActionParameter(expr)) {
        return getParamInstanceName(expr);
    }

    return ControlBodyTranslatorPSA::getParamName(expr);
}

// =====================EBPFTablePSA=============================
EBPFTablePSA::EBPFTablePSA(const EBPFProgram *program, const IR::TableBlock *table,
                           CodeGenInspector *codeGen)
    : EBPFTable(program, table, codeGen), implementation(nullptr) {
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

    tryEnableTableCache();
}

EBPFTablePSA::EBPFTablePSA(const EBPFProgram *program, CodeGenInspector *codeGen, cstring name)
    : EBPFTable(program, codeGen, name), implementation(nullptr) {}

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

    bool hasActionSelector =
        (implementation != nullptr && implementation->is<EBPFActionSelectorPSA>());

    // check if we have also selector key
    const IR::KeyElement *selectorKey = nullptr;
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
                "%1%: ActionSelector provided but there is no selector key", table->container);
    }
    if (!hasActionSelector && selectorKey != nullptr) {
        ::error(ErrorType::ERR_NOT_FOUND,
                "%1%: implementation not found, ActionSelector is required",
                selectorKey->matchType);
    }
    auto emptyGroupAction = table->container->properties->getProperty("psa_empty_group_action");
    if (!hasActionSelector && emptyGroupAction != nullptr) {
        ::warning(ErrorType::WARN_UNUSED, "%1%: unused property (ActionSelector not provided)",
                  emptyGroupAction);
    }
}

ActionTranslationVisitor *EBPFTablePSA::createActionTranslationVisitor(
    cstring valueName, const EBPFProgram *program) const {
    return new ActionTranslationVisitorPSA(program->to<EBPFPipeline>(), valueName, this);
}

void EBPFTablePSA::emitValueStructStructure(CodeBuilder *builder) {
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
            auto entries = getConstEntriesGroupedByMask();
            // A number of tuples is equal to number of unique masks
            unsigned nrOfTuples = entries.size();
            for (unsigned i = 0; i < nrOfTuples; i++) {
                builder->target->emitTableDecl(
                    builder, instanceName + "_tuple_" + std::to_string(i), TableHash,
                    "struct " + keyTypeName, "struct " + valueTypeName, size);
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
                                       program->arrayIndexType, cstring("struct ") + valueTypeName,
                                       1);
    }

    emitCacheInstance(builder);
}

void EBPFTablePSA::emitTypes(CodeBuilder *builder) {
    EBPFTable::emitTypes(builder);
    emitCacheTypes(builder);
}

/**
 * Order of emitting counters and meters affects generated layout of BPF map value.
 * Do not change this order!
 */
void EBPFTablePSA::emitDirectValueTypes(CodeBuilder *builder) {
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

void EBPFTablePSA::emitAction(CodeBuilder *builder, cstring valueName, cstring actionRunVariable) {
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
    const IR::EntriesList *entries = table->container->getEntries();
    if (entries == nullptr) return;

    if (isTernaryTable()) {
        emitTernaryConstEntriesInitializer(builder);
        return;
    }

    EBPFTablePSAInitializerCodeGen cg(program->refMap, program->typeMap, this);
    cg.setBuilder(builder);

    for (auto entry : entries->entries) {
        auto keyName = program->refMap->newName("key");
        auto valueName = program->refMap->newName("value");

        // construct key
        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", this->keyTypeName.c_str(), keyName.c_str());
        cg.generateKeyInitializer(entry);
        builder->endOfStatement(true);

        // construct value
        emitTableValue(builder, entry->action, valueName);

        // emit update
        auto ret = program->refMap->newName("ret");
        builder->emitIndent();
        builder->appendFormat("int %s = ", ret.c_str());
        builder->target->emitTableUpdate(builder, instanceName, keyName.c_str(), valueName.c_str());
        builder->newline();

        emitMapUpdateTraceMsg(builder, instanceName, ret);
    }
}

void EBPFTablePSA::emitDefaultActionInitializer(CodeBuilder *builder) {
    const IR::P4Table *t = table->container;
    const IR::Expression *defaultAction = t->getDefaultAction();
    auto actionName = getActionNameExpression(defaultAction);
    CHECK_NULL(actionName);
    if (actionName->path->name.originalName != P4::P4CoreLibrary::instance().noAction.name) {
        auto value = program->refMap->newName("value");
        emitTableValue(builder, defaultAction, value);
        auto ret = program->refMap->newName("ret");
        builder->emitIndent();
        builder->appendFormat("int %s = ", ret.c_str());
        builder->target->emitTableUpdate(builder, defaultActionMapName, program->zeroKey.c_str(),
                                         value.c_str());
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
    builder->target->emitTraceMessage(builder, msgStr, 1, returnCode.c_str());

    builder->blockEnd(false);
    builder->append(" else ");

    builder->blockStart();
    msgStr = Util::printf_format("Map initializer: Map (%s) update succeed", mapName,
                                 returnCode.c_str());
    builder->target->emitTraceMessage(builder, msgStr);
    builder->blockEnd(true);
}

const IR::PathExpression *EBPFTablePSA::getActionNameExpression(const IR::Expression *expr) const {
    BUG_CHECK(expr->is<IR::MethodCallExpression>(), "%1%: expected an action call", expr);
    auto mce = expr->to<IR::MethodCallExpression>();
    BUG_CHECK(mce->method->is<IR::PathExpression>(), "%1%: expected IR::PathExpression type",
              mce->method);
    return mce->method->to<IR::PathExpression>();
}

void EBPFTablePSA::emitTableValue(CodeBuilder *builder, const IR::Expression *expr,
                                  cstring valueName) {
    EBPFTablePSAInitializerCodeGen cg(program->refMap, program->typeMap, this);
    cg.setBuilder(builder);

    builder->emitIndent();
    builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), valueName.c_str());
    cg.generateValueInitializer(expr);
    builder->endOfStatement(true);
}

void EBPFTablePSA::emitLookupDefault(CodeBuilder *builder, cstring key, cstring value,
                                     cstring actionRunVariable) {
    if (implementation != nullptr) {
        builder->appendLine(
            "/* table with implementation has default action "
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
    if (implementation != nullptr) return false;
    return EBPFTable::dropOnNoMatchingEntryFound();
}

void EBPFTablePSA::emitTernaryConstEntriesInitializer(CodeBuilder *builder) {
    auto entriesGroupedByMask = getConstEntriesGroupedByMask();
    if (entriesGroupedByMask.empty()) return;

    std::vector<cstring> keyMasksNames;
    int tuple_id = 0;  // We have preallocated tuple maps with ids starting from 0

    // emit key head mask
    cstring headName = program->refMap->newName("key_mask");
    builder->emitIndent();
    builder->appendFormat("struct %s_mask %s = {0}", keyTypeName, headName);
    builder->endOfStatement(true);

    // emit key masks
    emitKeyMasks(builder, entriesGroupedByMask, keyMasksNames);

    builder->newline();

    // add head
    cstring valueMask = program->refMap->newName("value_mask");
    cstring nextMask = keyMasksNames[0];
    int noTupleId = -1;
    emitValueMask(builder, valueMask, nextMask, noTupleId);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("%s(0, 0, &%s, &%s, &%s, &%s, NULL, NULL)", addPrefixFunctionName,
                          tuplesMapName, prefixesMapName, headName, valueMask);
    builder->endOfStatement(true);
    builder->newline();

    // emit values + updates
    for (size_t i = 0; i < entriesGroupedByMask.size(); i++) {
        auto sameMaskEntries = entriesGroupedByMask[i];
        valueMask = program->refMap->newName("value_mask");
        std::vector<cstring> keyNames;
        std::vector<cstring> valueNames;
        cstring keysArray = program->refMap->newName("keys");
        cstring valuesArray = program->refMap->newName("values");
        cstring keyMaskVarName = keyMasksNames[i];

        if (entriesGroupedByMask.size() > i + 1) {
            nextMask = keyMasksNames[i + 1];
        } else {
            nextMask = nullptr;
        }
        emitValueMask(builder, valueMask, nextMask, tuple_id);
        builder->newline();
        emitKeysAndValues(builder, sameMaskEntries, keyNames, valueNames);

        // construct keys array
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("void *%s[] = {", keysArray);
        for (auto keyName : keyNames) builder->appendFormat("&%s,", keyName);
        builder->append("}");
        builder->endOfStatement(true);

        // construct values array
        builder->emitIndent();
        builder->appendFormat("void *%s[] = {", valuesArray);
        for (auto valueName : valueNames) builder->appendFormat("&%s,", valueName);
        builder->append("}");
        builder->endOfStatement(true);

        builder->newline();
        builder->emitIndent();
        builder->appendFormat("%s(%s, %s, &%s, &%s, &%s, &%s, %s, %s)", addPrefixFunctionName,
                              cstring::to_cstring(sameMaskEntries.size()),
                              cstring::to_cstring(tuple_id), tuplesMapName, prefixesMapName,
                              keyMaskVarName, valueMask, keysArray, valuesArray);
        builder->endOfStatement(true);

        tuple_id++;
    }
}

void EBPFTablePSA::emitKeysAndValues(CodeBuilder *builder, EntriesGroup_t &sameMaskEntries,
                                     std::vector<cstring> &keyNames,
                                     std::vector<cstring> &valueNames) {
    EBPFTablePSAInitializerCodeGen cg(program->refMap, program->typeMap, this);
    cg.setBuilder(builder);

    for (auto &entry : sameMaskEntries) {
        cstring keyName = program->refMap->newName("key");
        cstring valueName = program->refMap->newName("value");
        keyNames.push_back(keyName);
        valueNames.push_back(valueName);
        // construct key
        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", keyTypeName.c_str(), keyName.c_str());
        cg.generateKeyInitializer(entry.entry);
        builder->endOfStatement(true);

        // construct value
        emitTableValue(builder, entry.entry->action, valueName);

        // setup priority of the entry
        builder->emitIndent();
        builder->appendFormat("%s.priority = %u", valueName.c_str(), entry.priority);
        builder->endOfStatement(true);
    }
}

void EBPFTablePSA::emitKeyMasks(CodeBuilder *builder, EntriesGroupedByMask_t &entriesGroupedByMask,
                                std::vector<cstring> &keyMasksNames) {
    EBPFTablePSATernaryKeyMaskGenerator cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    for (auto sameMaskEntries : entriesGroupedByMask) {
        auto firstEntry = sameMaskEntries.front().entry;
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
            expr->apply(cg);
            builder->endOfStatement(true);
            builder->emitIndent();
            builder->appendFormat("__builtin_memcpy(%s, &%s, sizeof(%s))", keyFieldNamePtr,
                                  fieldName, fieldName);
            builder->endOfStatement(true);
            builder->emitIndent();
            builder->appendFormat("%s = %s + sizeof(%s)", keyFieldNamePtr, keyFieldNamePtr,
                                  fieldName);
            builder->endOfStatement(true);
        }
    }
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
 * For example four entries which have two different masks
 * will give as a result a list of two list (each with two entries).
 * @return a vector of vectors with const entries that have the same prefix
 */
EBPFTablePSA::EntriesGroupedByMask_t EBPFTablePSA::getConstEntriesGroupedByMask() {
    EntriesGroupedByMask_t result;
    const IR::EntriesList *entries = table->container->getEntries();

    if (!entries) return result;

    // Group entries by the same mask, container will do deduplication for us. The order of
    // entries will be changed but this is not a problem because of priority. Ebpf algorithm use
    // TSS, so every mask have to be tested and there is no strict requirements on masks order.
    // Priority of entries is equal to P4 program order (first defined has the highest priority).
    EBPFTablePSATernaryTableMaskGenerator maskGenerator(program->refMap, program->typeMap);
    std::unordered_map<cstring, std::vector<ConstTernaryEntryDesc>> entriesGroupedByMask;
    unsigned priority = entries->entries.size() + 1;
    for (auto entry : entries->entries) {
        cstring mask = maskGenerator.getMaskStr(entry);
        ConstTernaryEntryDesc desc;
        desc.entry = entry;
        desc.priority = priority--;
        entriesGroupedByMask[mask].emplace_back(desc);
    }

    // build results
    for (auto &vec : entriesGroupedByMask) {
        result.emplace_back(std::move(vec.second));
    }
    return result;
}

bool EBPFTablePSA::hasConstEntries() {
    const IR::EntriesList *entries = table->container->getEntries();
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
            "%trace_msg_tuple_not_found%", "        bpf_trace_message(\"Tuple not found\\n\");\n");
    } else {
        addPrefixFunc = addPrefixFunc.replace("%trace_msg_prefix_map_fail%", "");
        addPrefixFunc = addPrefixFunc.replace("%trace_msg_tuple_update_fail%", "");
        addPrefixFunc = addPrefixFunc.replace("%trace_msg_tuple_update_success%", "");
        addPrefixFunc = addPrefixFunc.replace("%trace_msg_tuple_not_found%", "");
    }

    return addPrefixFunc;
}

void EBPFTablePSA::tryEnableTableCache() {
    if (!program->options.enableTableCache) return;
    if (!isLPMTable() && !isTernaryTable()) return;
    if (!counters.empty() || !meters.empty()) {
        ::warning(ErrorType::WARN_UNSUPPORTED,
                  "%1%: table cache can't be enabled due to direct extern(s)",
                  table->container->name);
        return;
    }

    tableCacheEnabled = true;
    createCacheTypeNames(false, true);
}

void EBPFTablePSA::createCacheTypeNames(bool isCacheKeyType, bool isCacheValueType) {
    cacheTableName = instanceName + "_cache";

    cacheKeyTypeName = keyTypeName;
    if (isCacheKeyType) cacheKeyTypeName = keyTypeName + "_cache";

    cacheValueTypeName = valueTypeName;
    if (isCacheValueType) cacheValueTypeName = valueTypeName + "_cache";
}

void EBPFTablePSA::emitCacheTypes(CodeBuilder *builder) {
    if (!tableCacheEnabled) return;

    builder->emitIndent();
    builder->appendFormat("struct %s ", cacheValueTypeName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("struct %s value", valueTypeName.c_str());
    builder->endOfStatement(true);

    // additional metadata fields add at the end of this structure. This allows
    // to simpler conversion cache value to value used by table

    builder->emitIndent();
    builder->append("u8 hit");
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFTablePSA::emitCacheInstance(CodeBuilder *builder) {
    if (!tableCacheEnabled) return;

    // TODO: make cache size calculation more smart. Consider using annotation or compiler option.
    size_t cacheSize = std::max((size_t)1, size / 2);
    builder->target->emitTableDecl(builder, cacheTableName, TableHashLRU,
                                   "struct " + cacheKeyTypeName, "struct " + cacheValueTypeName,
                                   cacheSize);
}

void EBPFTablePSA::emitCacheLookup(CodeBuilder *builder, cstring key, cstring value) {
    cstring cacheVal = "cached_value";

    builder->appendFormat("struct %s* %s = NULL", cacheValueTypeName.c_str(), cacheVal.c_str());
    builder->endOfStatement(true);

    builder->target->emitTraceMessage(builder, "Control: trying table cache...");

    builder->emitIndent();
    builder->target->emitTableLookup(builder, cacheTableName, key, cacheVal);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", cacheVal.c_str());
    builder->blockStart();

    builder->target->emitTraceMessage(builder,
                                      "Control: table cache hit, skipping later lookup(s)");
    builder->emitIndent();
    builder->appendFormat("%s = &(%s->value)", value.c_str(), cacheVal.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("%s = %s->hit", program->control->hitVariable.c_str(), cacheVal.c_str());
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();

    builder->target->emitTraceMessage(builder, "Control: table cache miss, nevermind");
    builder->emitIndent();

    // Do not end block here because we need lookup for (default) value
    // and set hit variable at this indent level which is done in the control block
}

void EBPFTablePSA::emitCacheUpdate(CodeBuilder *builder, cstring key, cstring value) {
    cstring cacheUpdateVarName = "cache_update";

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", value.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendLine("/* update table cache */");

    builder->emitIndent();
    builder->appendFormat("struct %s %s = {0}", cacheValueTypeName.c_str(),
                          cacheUpdateVarName.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("%s.hit = %s", cacheUpdateVarName.c_str(),
                          program->control->hitVariable.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("__builtin_memcpy((void *) &(%s.value), (void *) %s, sizeof(struct %s))",
                          cacheUpdateVarName.c_str(), value.c_str(), valueTypeName.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->target->emitTableUpdate(builder, cacheTableName, key, cacheUpdateVarName);
    builder->newline();

    builder->target->emitTraceMessage(builder, "Control: table cache updated");

    builder->blockEnd(true);
}

}  // namespace EBPF
