#ifndef DPDK_CONTROL_PLANE_BFRUNTIME_ARCH_HANDLER_H_
#define DPDK_CONTROL_PLANE_BFRUNTIME_ARCH_HANDLER_H_

#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include "p4/config/dpdk/p4info.pb.h"

#include "control-plane/bfruntime.h"

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "control-plane/typeSpecConverter.h"

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"

#include "midend/eliminateTypedefs.h"

#include "p4/config/v1/p4info.pb.h"

using P4::ControlPlaneAPI::P4RuntimeSymbolTableIface;
using P4::ControlPlaneAPI::P4RuntimeSymbolType;
using P4::ControlPlaneAPI::p4rt_id_t;
using P4::ControlPlaneAPI::TypeSpecConverter;

using P4::ReferenceMap;
using P4::TypeMap;

namespace p4configv1 = ::p4::config::v1;

namespace Helpers = P4::ControlPlaneAPI::Helpers;

namespace DPDK {

/// Extends P4RuntimeSymbolType for the Tofino extern types.
class SymbolType final : public P4RuntimeSymbolType {
 public:
    SymbolType() = delete;

    static P4RuntimeSymbolType ACTION_SELECTOR() {
        return P4RuntimeSymbolType::make(barefoot::P4Ids::ACTION_SELECTOR);
    }
};

/// The information about an action profile which is necessary to generate its
/// serialized representation.
struct ActionSelector {
    const cstring name;  // The fully qualified external name of this action selector.
    const boost::optional<cstring> actionProfileName;  // If not known, we will generate
                         // an action profile instance.
    const int64_t size;  // TODO(hanw): size does not make sense with new ActionSelector P4 extern
    const int64_t maxGroupSize;
    const int64_t numGroups;
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // profile declaration.
    bool selSuffix;

    p4rt_id_t getId(const P4RuntimeSymbolTableIface& symbols,
                                    const cstring blockPrefix = "") const {
        auto symName = prefix(blockPrefix, name);
        auto symSelectorName = (selSuffix) ? symName + "_sel" : symName;
        if (actionProfileName) {
            return symbols.getId(SymbolType::ACTION_SELECTOR(), symName);
        }
        return symbols.getId(SymbolType::ACTION_SELECTOR(), symSelectorName);
    }
};


/// Parent class for BFRuntimeArchHandlerV1Model and BFRuntimeArchHandlerPSA; it
/// includes all the common code between the two architectures (which is only
/// dependent on the @arch template parameter.
template <Arch arch>
class BFRuntimeArchHandlerCommon: public P4::ControlPlaneAPI::P4RuntimeArchHandlerIface {
 protected:
    using ArchCounterExtern = CounterExtern<arch>;
    using CounterTraits = Helpers::CounterlikeTraits<ArchCounterExtern>;
    using ArchMeterExtern = MeterExtern<arch>;
    using MeterTraits = Helpers::CounterlikeTraits<ArchMeterExtern>;

    BFRuntimeArchHandlerCommon(ReferenceMap* refMap, TypeMap* typeMap,
                            const IR::ToplevelBlock* evaluatedProgram)
        : refMap(refMap), typeMap(typeMap), evaluatedProgram(evaluatedProgram) {
    }

    // Overridden function call from base ArchIFace class. Table blocks are
    // added entirely in the frontend and this call ensures we have our own
    // implementation of control plane name generation (with the pipe prefix).
    // This is relevant in multi pipe scenarios where a table (its control) is
    // shared across pipes and needs fully qualified unique names.
    cstring getControlPlaneName(const IR::Block* block) override {
        return getControlPlaneName(block, nullptr);
    }

    cstring getControlPlaneName(const IR::Block* block, const IR::IDeclaration *decl) {
        auto declName = decl ? decl->controlPlaneName() : "";
        return getFullyQualifiedName(block, declName);
    }

    // Given a block get its block prefix (usually pipe name) generate in the
    // blockNamePrefixMap and prefix it to the control plane name. This name is
    // unique to the resource. For multi pipe scenarios this becomes relevant
    // since a resource can be shared across pipes but need to be addressed by
    // the control plane as separate resources.
    // Note, in some cases like multiparser blocks, the parser names are always
    // arch names since the instantiations are always anonymous. In such cases
    // we skip the control plane name  [ DRV-2939 ]
    // E.g.
    // P4:
    // IngressParsers(IgCPUParser(), IgNetworkParser()) ig_pipe0;
    //  MultiParserPipeline(ig_pipe0, SwitchIngress(), SwitchIngressDeparser(),
    //                      eg_pipe0, SwitchEgress(), SwitchEgressDeparser()) pipe0;
    // BF-RT JSON : pipe0.ig_pipe0.prsr0
    // CTXT JSON : ig_pipe0.prsr0
    cstring getFullyQualifiedName(const IR::Block *block, const cstring name,
                                                bool skip_control_plane_name = false) {
        cstring block_name = "";
        cstring control_plane_name = "";
        cstring full_name = getBlockNamePrefix(block);
        if (!skip_control_plane_name) {
            if (auto cont = block->getContainer()) {
                block_name = cont->getName();
                control_plane_name = cont->controlPlaneName();
                full_name = prefix(full_name, control_plane_name);
            }
        }

        if (!name.isNullOrEmpty())
            full_name = prefix(full_name, name);
        LOG5("Block : " << block
                << ", block_name: " << block_name
                << ", block_name_prefix: " << getBlockNamePrefix(block)
                << ", control_plane_name : " << control_plane_name
                << ", name: "       << name
                << ", fqname: "     << full_name);
        return full_name;
    }

    virtual cstring getBlockNamePrefix(const IR::Block *) {
        return defaultPipeName;
    }

    void collectTableProperties(P4RuntimeSymbolTableIface* symbols,
                                const IR::TableBlock* tableBlock) override {
        CHECK_NULL(tableBlock);
        bool isConstructedInPlace = false;

        using BFN::getExternInstanceFromPropertyByTypeName;

        {
            // Only collect the symbol if the action profile / selector is
            // constructed in place. Otherwise it will be collected by
            // collectExternInstance, which will avoid duplicates.
            auto table = tableBlock->container;
            auto action_profile = getExternInstanceFromPropertyByTypeName(
                table, implementationString, "ActionProfile",
                refMap, typeMap, &isConstructedInPlace);
            if (action_profile) {
                cstring tableName = *action_profile->name;
                tableName = prefix(getBlockNamePrefix(tableBlock), tableName);
                if (isConstructedInPlace)
                    symbols->add(SymbolType::ACTION_PROFILE(), tableName);
            }
            auto action_selector = getExternInstanceFromPropertyByTypeName(
                table, implementationString, "ActionSelector",
                refMap, typeMap, &isConstructedInPlace);
            if (action_selector) {
                cstring tableName = *action_selector->name;
                tableName = prefix(getBlockNamePrefix(tableBlock), tableName);
                if (action_selector->substitution.lookupByName("size")) {
                    std::string selectorName(tableName + "_sel");
                    symbols->add(SymbolType::ACTION_SELECTOR(), selectorName);
                    symbols->add(SymbolType::ACTION_PROFILE(), tableName);
                } else {
                    symbols->add(SymbolType::ACTION_SELECTOR(), tableName);
                }
            }
        }
        // In TNA direct counters / meters cannot be constructed in place in the
        // table declaration because the count / execute method has to be
        // called.
    }

    void collectExternInstanceCommon(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) {
        CHECK_NULL(externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        // Skip externs instantiated inside table declarations (as properties);
        // that should only apply to action profiles / selectors since direct
        // resources cannot be constructed in place for TNA.
        if (decl == nullptr) return;

        auto symName = getControlPlaneName(externBlock, decl);
        if (externBlock->type->name == "Counter") {
            symbols->add(SymbolType::COUNTER(), symName);
        } else if (externBlock->type->name == "DirectCounter") {
            symbols->add(SymbolType::DIRECT_COUNTER(), symName);
        } else if (externBlock->type->name == "Meter") {
            symbols->add(SymbolType::METER(), symName);
        } else if (externBlock->type->name == "DirectMeter") {
            symbols->add(SymbolType::DIRECT_METER(), symName);
        } else if (externBlock->type->name == "ActionProfile") {
            symbols->add(SymbolType::ACTION_PROFILE(), symName);
        } else if (externBlock->type->name == "ActionSelector") {
            if (externBlock->findParameterValue("size")) {
                std::string selectorName(symName + "_sel");
                symbols->add(SymbolType::ACTION_SELECTOR(), selectorName);
                symbols->add(SymbolType::ACTION_PROFILE(), symName);
            } else {
                symbols->add(SymbolType::ACTION_SELECTOR(), symName);
            }
        } else if (externBlock->type->name == "Digest") {
            symbols->add(SymbolType::DIGEST(), symName);
        } else if (externBlock->type->name == "Register") {
            symbols->add(SymbolType::REGISTER(), symName);
        } else if (externBlock->type->name == "DirectRegister") {
            symbols->add(SymbolType::DIRECT_REGISTER(), symName);
        } else if (externBlock->type->name == "Hash") {
            symbols->add(SymbolType::HASH(), symName);
        }
    }

    void collectExternFunction(P4RuntimeSymbolTableIface*,
                               const P4::ExternFunction*) override {}

    void collectParserSymbols(P4RuntimeSymbolTableIface* symbols,
                              const IR::ParserBlock* parserBlock) {
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);

        // Collect any extern functions it may invoke.

        for (auto s : parser->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                symbols->add(SymbolType::VALUE_SET(), inst->controlPlaneName());
            }
        }
    }

    void collectExtra(P4RuntimeSymbolTableIface* symbols) override {
        // Collect value sets. This step is required because value set support
        // in "standard" P4Info is currently insufficient.
        // Also retrieve user-provided name for ig_intr_md parameter in ingress
        // parser to use as key name for phase0 table.
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ParserBlock>()) return;
            collectParserSymbols(symbols, block->to<IR::ParserBlock>());
        });
    }

    void addTablePropertiesCommon(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock,
                            cstring blockPrefix = "") {
        CHECK_NULL(tableBlock);
        auto tableDeclaration = tableBlock->container;

        using P4::ControlPlaneAPI::Helpers::isExternPropertyConstructedInPlace;

        auto p4RtTypeInfo = p4info->mutable_type_info();

        auto actionProfile = getActionProfile(tableDeclaration, refMap, typeMap);
        auto actionSelector = getActionSelector(tableDeclaration, refMap, typeMap);
        auto directCounter = Helpers::getDirectCounterlike<ArchCounterExtern>(
            tableDeclaration, refMap, typeMap);
        auto directMeter = Helpers::getDirectCounterlike<ArchMeterExtern>(
            tableDeclaration, refMap, typeMap);
        auto directRegister = getDirectRegister(
            tableDeclaration, refMap, typeMap, p4RtTypeInfo);
        auto supportsTimeout = getSupportsTimeout(tableDeclaration);

        if (actionProfile) {
            auto id = actionProfile->getId(symbols, blockPrefix);
            table->set_implementation_id(id);
            if (isExternPropertyConstructedInPlace(tableDeclaration, implementationString)) {
                addActionProfile(symbols, p4info, *actionProfile, blockPrefix);
            }
        }

        if (actionSelector) {
            auto id = actionSelector->getId(symbols, blockPrefix);
            table->set_implementation_id(id);
            if (isExternPropertyConstructedInPlace(tableDeclaration, implementationString)) {
                addActionSelector(symbols, p4info, *actionSelector, blockPrefix);
            }
        }

        // Direct resources are handled here. There is no risk to create
        // duplicates as direct resources cannot be shared across tables. We
        // could also handle those in addExternInstance but it would not be very
        // convenient to get a handle on the parent table (the parent table's id
        // is included in the P4Info message).
        if (directCounter) {
            auto id = symbols.getId(SymbolType::DIRECT_COUNTER(),
                    prefix(blockPrefix, directCounter->name));
            table->add_direct_resource_ids(id);
            addCounter(symbols, p4info, *directCounter, blockPrefix);
        }

        if (directMeter) {
            auto id = symbols.getId(SymbolType::DIRECT_METER(),
                    prefix(blockPrefix, directMeter->name));
            table->add_direct_resource_ids(id);
            addMeter(symbols, p4info, *directMeter, blockPrefix);
        }

        if (directRegister) {
            auto id = symbols.getId(SymbolType::DIRECT_REGISTER(),
                    prefix(blockPrefix, directRegister->name));
            table->add_direct_resource_ids(id);
            addRegister(symbols, p4info, *directRegister, blockPrefix);
        }

        // TODO(antonin): idle timeout may change for TNA in the future and we
        // may need to rely on P4Info table specific extensions.
        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    /// @return true if @table's 'idle_timeout' property exists and is
    /// true. This indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table* table) {
        auto timeout = table->properties->getProperty("idle_timeout");
        if (timeout == nullptr) return false;
        if (!timeout->value->is<IR::ExpressionValue>()) {
            ::error("Unexpected value %1% for idle_timeout on table %2%",
                    timeout, table);
            return false;
        }

        auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error("Unexpected non-boolean value %1% for idle_timeout "
                    "property on table %2%", timeout, table);
            return false;
        }

        return expr->to<IR::BoolLiteral>()->value;
    }

    /// @return the direct register associated with @table, if it has one, or
    /// boost::none otherwise.
    static boost::optional<Register> getDirectRegister(
        const IR::P4Table* table,
        ReferenceMap* refMap,
        TypeMap* typeMap,
        p4configv1::P4TypeInfo* p4RtTypeInfo) {
        auto directRegisterInstance = Helpers::getExternInstanceFromProperty(
            table, "registers", refMap, typeMap);
        if (!directRegisterInstance) return boost::none;
        return Register::fromDirect(
            *directRegisterInstance, table, refMap, typeMap, p4RtTypeInfo);
    }

    void addExternInstance(const P4RuntimeSymbolTableIface&,
                           p4configv1::P4Info*, const IR::ExternBlock*) override {
    }

    void addExternInstanceCommon(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock,
                           cstring pipeName = "") {
        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        // Skip externs instantiated inside table declarations (constructed in
        // place as properties).
        if (decl == nullptr) return;

        using Helpers::Counterlike;

        auto p4RtTypeInfo = p4info->mutable_type_info();
        // Direct resources are handled by addTableProperties.
        if (externBlock->type->name == "ActionProfile") {
            auto actionProfile = getActionProfile(externBlock);
            if (actionProfile) addActionProfile(symbols, p4info, *actionProfile, pipeName);
        } else if (externBlock->type->name == "ActionSelector") {
            auto actionSelector = getActionSelector(externBlock);
            if (actionSelector) addActionSelector(symbols, p4info, *actionSelector, pipeName);
        } else if (externBlock->type->name == "Counter") {
            auto counter = Counterlike<ArchCounterExtern>::from(externBlock,
                    refMap, typeMap, p4RtTypeInfo);
            if (counter) addCounter(symbols, p4info, *counter, pipeName);
        } else if (externBlock->type->name == "Meter") {
            auto meter = Counterlike<ArchMeterExtern>::from(externBlock,
                    refMap, typeMap, p4RtTypeInfo);
            if (meter) addMeter(symbols, p4info, *meter, pipeName);
        } else if (externBlock->type->name == "Digest") {
            auto digest = getDigest(decl, p4RtTypeInfo);
            if (digest) addDigest(symbols, p4info, *digest, pipeName);
        } else if (externBlock->type->name == "Register") {
            auto register_ = Register::from(externBlock, refMap, typeMap, p4RtTypeInfo);
            if (register_) addRegister(symbols, p4info, *register_, pipeName);
        } else if (externBlock->type->name == "Hash") {
            auto dynHash = getDynHash(decl, p4RtTypeInfo);
            if (dynHash) addDynHash(symbols, p4info, *dynHash, pipeName);
        }
    }

    void addExternFunction(const P4RuntimeSymbolTableIface&,
                           p4configv1::P4Info*, const P4::ExternFunction*) override {}

    void addValueSet(const P4RuntimeSymbolTableIface& symbols,
                     ::p4::config::v1::P4Info* p4info,
                     const ValueSet& valueSetInstance) {
        ::barefoot::ValueSet valueSet;
        valueSet.set_size(valueSetInstance.size);
        valueSet.mutable_type_spec()->CopyFrom(*valueSetInstance.typeSpec);
        addP4InfoExternInstance(
            symbols, SymbolType::VALUE_SET(), "ValueSet",
            valueSetInstance.name, valueSetInstance.annotations, valueSet,
            p4info);
    }

    void analyzeParser(const P4RuntimeSymbolTableIface& symbols,
                       ::p4::config::v1::P4Info* p4info,
                       const IR::ParserBlock* parserBlock) {
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);

        for (auto s : parser->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                auto valueSet = ValueSet::from(inst->controlPlaneName(), inst, refMap,
                        typeMap, p4info->mutable_type_info());
                if (valueSet) addValueSet(symbols, p4info, *valueSet);
            }
        }
    }

    void postAdd(const P4RuntimeSymbolTableIface& symbols,
                 ::p4::config::v1::P4Info* p4info) override {
        // Generates Tofino-specific ValueSet P4Info messages. This step is
        // required because value set support in "standard" P4Info is currently
        // insufficient: the standard ValueSet message restricts the element
        // type to a simple binary string (P4Runtime v1.0 limitation).
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ParserBlock>()) return;
            analyzeParser(symbols, p4info, block->to<IR::ParserBlock>());
        });
    }

    boost::optional<Digest> getDigest(const IR::Declaration_Instance* decl,
                                      p4configv1::P4TypeInfo* p4RtTypeInfo) {
        std::vector<const P4::ExternMethod*> packCalls;
        // Check that the pack method is called exactly once on the digest
        // instance. The type of the data being packed used to be a type
        // parameter of the pack method itself, and not a type parameter of the
        // extern, so the we used to require a reference to the P4::ExternMethod
        // for the pack call in order to produce the P4Info type spec. This is
        // no longer the case but we keep this code as a sanity check.
        forAllExternMethodCalls(decl, [&](const P4::ExternMethod* method) {
            packCalls.push_back(method); });
        if (packCalls.size() == 0) return boost::none;
        if (packCalls.size() > 1) {
            ::error("Expected single call to pack for digest instance '%1%'", decl);
            return boost::none;
        }
        LOG4("Found 'pack' method call for digest instance " << decl->controlPlaneName());

        BUG_CHECK(decl->type->is<IR::Type_Specialized>(),
                  "%1%: expected Type_Specialized", decl->type);
        auto type = decl->type->to<IR::Type_Specialized>();
        BUG_CHECK(type->arguments->size() == 1, "%1%: expected one type argument", decl);
        auto typeArg = type->arguments->at(0);
        auto typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeArg, p4RtTypeInfo);
        BUG_CHECK(typeSpec != nullptr,
                  "P4 type %1% could not be converted to P4Info P4DataTypeSpec");
        return Digest{decl->controlPlaneName(), typeSpec, decl->to<IR::IAnnotated>()};
    }

    boost::optional<DynHash> getDynHash(const IR::Declaration_Instance* decl,
                                      p4configv1::P4TypeInfo* p4RtTypeInfo) {
        std::vector<const P4::ExternMethod*> hashCalls;
        // Get Hash Calls in the program for the declaration.
        forAllExternMethodCalls(decl, [&](const P4::ExternMethod* method) {
            hashCalls.push_back(method); });
        if (hashCalls.size() == 0) return boost::none;
        if (hashCalls.size() > 1) {
            ::warning("Expected single call to get for hash instance '%1%'."
                    "Control plane API is not generated for this hash call", decl);
            return boost::none;
        }
        LOG4("Found 'get' method call for hash instance " << decl->controlPlaneName());

        // Extract typeArgs and field Names to be passed on through dynHash
        // instance
        if (auto *call = hashCalls[0]->expr->to<IR::MethodCallExpression>()) {
            int hashWidth = 0;
            if (auto t = call->type->to<IR::Type_Bits>()) {
                hashWidth = t->width_bits();
            }

            auto fieldListArg = call->arguments->at(0);
            LOG4("FieldList for Hash: " << fieldListArg);
            auto *typeArgs = new IR::Vector<IR::Type>();
            std::vector<DynHash::hashField> hashFieldInfo;
            if (auto fieldListExpr = fieldListArg->expression->to<IR::ListExpression>()) {
                for (auto f : fieldListExpr->components) {
                    if (auto c = f->to<IR::Concat>()) {
                        for (auto e : convertConcatToList(c)) {
                            hashFieldInfo.push_back({ e->toString(), e->is<IR::Constant>() });
                            typeArgs->push_back(e->type);
                        }
                        continue;
                    }
                    hashFieldInfo.push_back({ f->toString(), f->is<IR::Constant>() });
                    auto t = f->type->is<IR::Type_SerEnum>() ?
                        f->type->to<IR::Type_SerEnum>()->type : f->type;
                    typeArgs->push_back(t);
                }
            }
            auto *typeList = new IR::Type_List(*typeArgs);
            auto typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeList, p4RtTypeInfo);
            BUG_CHECK(typeSpec != nullptr,
                  "P4 type %1% could not be converted to P4Info P4DataTypeSpec");
            return DynHash{decl->controlPlaneName(), typeSpec,
                decl->to<IR::IAnnotated>(), hashFieldInfo, hashWidth};
        }
        return boost::none;
    }

    /// @return the action profile referenced in @table's implementation
    /// property, if it has one, or boost::none otherwise.
    boost::optional<ActionProfile>
    getActionProfile(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
        using Helpers::getExternInstanceFromProperty;
        auto instance = getExternInstanceFromProperty(
                table, implementationString, refMap, typeMap);
        if (!instance) return boost::none;
        if (instance->type->name != "ActionProfile") return boost::none;
        auto size = instance->substitution.lookupByName("size")->expression;
        // size is a bit<32> compile-time value
        BUG_CHECK(size->is<IR::Constant>(), "Non-constant size");
        return ActionProfile{*instance->name,
                             size->to<IR::Constant>()->asInt(),
                             getTableImplementationAnnotations(table, refMap)};
    }

    /// @return the action profile corresponding to @instance.
    static boost::optional<ActionProfile>
    getActionProfile(const IR::ExternBlock* instance) {
        auto decl = instance->node->to<IR::IDeclaration>();
        auto size = instance->getParameterValue("size");
        BUG_CHECK(size->is<IR::Constant>(), "Non-constant size");
        return ActionProfile{decl->controlPlaneName(),
                             size->to<IR::Constant>()->asInt(),
                             decl->to<IR::IAnnotated>()};
    }

    /// @return the action profile referenced in @table's implementation
    /// property, if it has one, or boost::none otherwise.
    boost::optional<ActionSelector>
    getActionSelector(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
        using BFN::getExternInstanceFromPropertyByTypeName;
        auto action_selector = getExternInstanceFromPropertyByTypeName(
            table, implementationString, "ActionSelector", refMap, typeMap);
        if (!action_selector) return boost::none;
        // TODO(hanw): remove legacy code
        // used to support deprecated ActionSelector constructor.
        if (action_selector->substitution.lookupByName("size")) {
            auto size = action_selector->substitution.lookupByName("size")->expression;
            BUG_CHECK(size->is<IR::Constant>(), "Non-constant size");
            return ActionSelector{*action_selector->name,
                                  boost::none,
                                  size->to<IR::Constant>()->asInt(),
                                  defaultMaxGroupSize,
                                  size->to<IR::Constant>()->asInt(),
                                  getTableImplementationAnnotations(table, refMap),
                                  true /* _sel suffix */ };
        }
        auto maxGroupSize =
            action_selector->substitution.lookupByName("max_group_size")->expression;
        auto numGroups =
            action_selector->substitution.lookupByName("num_groups")->expression;
        // size is a bit<32> compile-time value
        BUG_CHECK(maxGroupSize->is<IR::Constant>(), "Non-constant max group size");
        BUG_CHECK(numGroups->is<IR::Constant>(), "Non-constant num groups");
        return ActionSelector{*action_selector->name,
                              *action_selector->name,
                              -1  /* size */,
                              maxGroupSize->to<IR::Constant>()->asInt(),
                              numGroups->to<IR::Constant>()->asInt(),
                              getTableImplementationAnnotations(table, refMap),
                              false /* _sel suffix */ };
    }

    boost::optional<ActionSelector>
    getActionSelector(const IR::ExternBlock* instance) {
        auto actionSelDecl = instance->node->to<IR::IDeclaration>();
        // to be deleted, used to support deprecated ActionSelector constructor.
        if (instance->findParameterValue("size")) {
            auto size = instance->getParameterValue("size");
            BUG_CHECK(size->is<IR::Constant>(), "Non-constant size");
            return ActionSelector{actionSelDecl->controlPlaneName(),
                                  boost::none,
                                  size->to<IR::Constant>()->asInt(),
                                  defaultMaxGroupSize,
                                  size->to<IR::Constant>()->asInt(),
                                  actionSelDecl->to<IR::IAnnotated>(),
                                  false  /* sel suffix */};
        }
        auto maxGroupSize = instance->getParameterValue("max_group_size");
        auto numGroups = instance->getParameterValue("num_groups");
        BUG_CHECK(maxGroupSize->is<IR::Constant>(), "Non-constant max group size");
        BUG_CHECK(numGroups->is<IR::Constant>(), "Non-constant num groups");
        auto action_profile = instance->getParameterValue("action_profile");
        auto action_profile_decl =
            action_profile->to<IR::ExternBlock>()->node->to<IR::IDeclaration>();
        return ActionSelector{actionSelDecl->controlPlaneName(),
            cstring::to_cstring(action_profile_decl->controlPlaneName()),
            -1  /* size */,
            maxGroupSize->to<IR::Constant>()->asInt(),
            numGroups->to<IR::Constant>()->asInt(),
            actionSelDecl->to<IR::IAnnotated>(),
            false  /* sel suffix */};
    }

    static p4configv1::Extern* getP4InfoExtern(P4RuntimeSymbolType typeId,
                                               cstring typeName,
                                               p4configv1::P4Info* p4info) {
        for (auto& externType : *p4info->mutable_externs()) {
            if (externType.extern_type_id() == static_cast<p4rt_id_t>(typeId))
                return &externType;
        }
        auto* externType = p4info->add_externs();
        externType->set_extern_type_id(static_cast<p4rt_id_t>(typeId));
        externType->set_extern_type_name(typeName);
        return externType;
    }

    static void addP4InfoExternInstance(const P4RuntimeSymbolTableIface& symbols,
                                        P4RuntimeSymbolType typeId, cstring typeName,
                                        cstring name, const IR::IAnnotated* annotations,
                                        const ::google::protobuf::Message& message,
                                        p4configv1::P4Info* p4info) {
        auto* externType = getP4InfoExtern(typeId, typeName, p4info);
        auto* externInstance = externType->add_instances();
        auto* pre = externInstance->mutable_preamble();
        pre->set_id(symbols.getId(typeId, name));
        pre->set_name(name);
        pre->set_alias(symbols.getAlias(name));
        Helpers::addAnnotations(pre, annotations);
        Helpers::addDocumentation(pre, annotations);
        externInstance->mutable_info()->PackFrom(message);
    }

    void addDigest(const P4RuntimeSymbolTableIface& symbols,
                   p4configv1::P4Info* p4Info,
                   const Digest& digestInstance,
                   cstring pipeName = "") {
        ::barefoot::Digest digest;
        digest.mutable_type_spec()->CopyFrom(*digestInstance.typeSpec);
        auto digestName = prefix(pipeName, digestInstance.name);
        addP4InfoExternInstance(
            symbols, SymbolType::DIGEST(), "Digest",
            digestName, digestInstance.annotations, digest,
            p4Info);
    }

    void addDynHash(const P4RuntimeSymbolTableIface& symbols,
                   p4configv1::P4Info* p4Info,
                   const DynHash& dynHashInstance,
                   cstring pipeName = "") {
        ::barefoot::DynHash dynHash;
        dynHash.set_hash_width(dynHashInstance.hashWidth);
        dynHash.mutable_type_spec()->CopyFrom(*dynHashInstance.typeSpec);
        auto dynHashName = prefix(pipeName, dynHashInstance.name);
        for (const auto& f : dynHashInstance.hashFieldInfo) {
            auto newF = dynHash.add_field_infos();
            newF->set_field_name(f.hashFieldName);
            newF->set_is_constant(f.isConstant);
        }
        addP4InfoExternInstance(
            symbols, SymbolType::HASH(), "DynHash", dynHashName,
            dynHashInstance.annotations, dynHash, p4Info);
    }

    // For Registers, the table name should have the associated pipe prefix but
    // the data field names should not since they have local scope.
    void addRegister(const P4RuntimeSymbolTableIface& symbols,
                     p4configv1::P4Info* p4Info,
                     const Register& registerInstance,
                     cstring pipeName = "") {
        auto registerName = prefix(pipeName, registerInstance.name);
        if (registerInstance.size == 0) {
            ::barefoot::DirectRegister register_;
            register_.mutable_type_spec()->CopyFrom(*registerInstance.typeSpec);
            register_.set_data_field_name(registerInstance.name);
            addP4InfoExternInstance(
                symbols, SymbolType::DIRECT_REGISTER(), "DirectRegister",
                registerName, registerInstance.annotations, register_,
                p4Info);
        } else {
            ::barefoot::Register register_;
            register_.set_size(registerInstance.size);
            register_.mutable_type_spec()->CopyFrom(*registerInstance.typeSpec);
            register_.set_data_field_name(registerInstance.name);
            addP4InfoExternInstance(
                symbols, SymbolType::REGISTER(), "Register",
                registerName, registerInstance.annotations, register_,
                p4Info);
        }
    }

    /// Set common fields between barefoot::Counter and barefoot::DirectCounter.
    template <typename Kind>
    void setCounterCommon(Kind *counter,
                          const Helpers::Counterlike<ArchCounterExtern>& counterInstance) {
        auto counter_spec = counter->mutable_spec();
        counter_spec->set_unit(CounterTraits::mapUnitName(counterInstance.unit));
    }

    void addCounter(const P4RuntimeSymbolTableIface& symbols,
                    p4configv1::P4Info* p4Info,
                    const Helpers::Counterlike<ArchCounterExtern>& counterInstance,
                    const cstring blockPrefix = "") {
        if (counterInstance.table) {
            ::barefoot::DirectCounter counter;
            setCounterCommon(&counter, counterInstance);
            auto tableName = prefix(blockPrefix, *counterInstance.table);
            auto counterName = prefix(blockPrefix, counterInstance.name);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), tableName);
            counter.set_direct_table_id(tableId);
            addP4InfoExternInstance(
                symbols, SymbolType::DIRECT_COUNTER(),
                Helpers::CounterlikeTraits<ArchCounterExtern>::directTypeName(),
                counterName, counterInstance.annotations, counter,
                p4Info);
        } else {
            ::barefoot::Counter counter;
            setCounterCommon(&counter, counterInstance);
            counter.set_size(counterInstance.size);
            auto counterName = prefix(blockPrefix, counterInstance.name);
            addP4InfoExternInstance(
                symbols, SymbolType::COUNTER(),
                Helpers::CounterlikeTraits<ArchCounterExtern>::typeName(),
                counterName, counterInstance.annotations, counter,
                p4Info);
        }
    }

    /// Set common fields between barefoot::Meter and barefoot::DirectMeter.
    template <typename Kind>
    void setMeterCommon(Kind *meter,
                        const Helpers::Counterlike<ArchMeterExtern>& meterInstance) {
        using ::barefoot::MeterSpec;
        using Helpers::CounterlikeTraits;
        auto meter_spec = meter->mutable_spec();
        if (colorAwareMeters.find(meterInstance.name) != colorAwareMeters.end()) {
            meter_spec->set_type(MeterSpec::COLOR_AWARE);
        } else {
            meter_spec->set_type(MeterSpec::COLOR_UNAWARE);
        }
        meter_spec->set_unit(CounterlikeTraits<ArchMeterExtern>::mapUnitName(meterInstance.unit));
    }

    void addMeter(const P4RuntimeSymbolTableIface& symbols,
                  p4configv1::P4Info* p4Info,
                  const Helpers::Counterlike<ArchMeterExtern>& meterInstance,
                  const cstring blockPrefix = "") {
        auto meterName = prefix(blockPrefix, meterInstance.name);
        if (meterInstance.table) {
            ::barefoot::DirectMeter meter;
            setMeterCommon(&meter, meterInstance);
            auto tableName = prefix(blockPrefix, *meterInstance.table);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), tableName);
            meter.set_direct_table_id(tableId);
            addP4InfoExternInstance(
                symbols, SymbolType::DIRECT_METER(),
                Helpers::CounterlikeTraits<ArchMeterExtern>::directTypeName(),
                meterName, meterInstance.annotations, meter, p4Info);
        } else {
            ::barefoot::Meter meter;
            setMeterCommon(&meter, meterInstance);
            meter.set_size(meterInstance.size);
            addP4InfoExternInstance(
                symbols, SymbolType::METER(),
                Helpers::CounterlikeTraits<ArchMeterExtern>::typeName(),
                meterName, meterInstance.annotations, meter, p4Info);
        }
    }

    void addActionProfile(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          const ActionProfile& actionProfile,
                          cstring pipeName = "") {
        ::barefoot::ActionProfile profile;
        profile.set_size(actionProfile.size);
        auto tablesIt = actionProfilesRefs.find(actionProfile.name);
        if (tablesIt != actionProfilesRefs.end()) {
            for (const auto& table : tablesIt->second) {
                cstring tableName = table;
                if (!pipeName.isNullOrEmpty())
                    tableName = prefix(pipeName, tableName);
                profile.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), tableName));
            }
        }
        auto actionProfileName = prefix(pipeName, actionProfile.name);
        addP4InfoExternInstance(symbols, SymbolType::ACTION_PROFILE(), "ActionProfile",
                                actionProfileName, actionProfile.annotations, profile,
                                p4Info);
    }

    virtual void addActionSelector(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          const ActionSelector& actionSelector,
                          cstring blockPrefix = "") {
        ::barefoot::ActionSelector selector;
        selector.set_max_group_size(actionSelector.maxGroupSize);
        selector.set_num_groups(actionSelector.numGroups);
        cstring actionSelectorName = prefix(blockPrefix, actionSelector.name);
        if (actionSelector.actionProfileName) {
            selector.set_action_profile_id(symbols.getId(
                SymbolType::ACTION_PROFILE(),
                prefix(blockPrefix, *actionSelector.actionProfileName)));
            auto tablesIt = actionProfilesRefs.find(actionSelector.name);
            if (tablesIt != actionProfilesRefs.end()) {
                for (const auto& table : tablesIt->second) {
                    cstring tableName = prefix(blockPrefix, table);
                    selector.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), tableName));
                }
            }
            addP4InfoExternInstance(symbols, SymbolType::ACTION_SELECTOR(), "ActionSelector",
                    actionSelectorName, actionSelector.annotations,
                    selector, p4Info);
        } else {
            ::barefoot::ActionProfile profile;
            profile.set_size(actionSelector.size);
            auto tablesIt = actionProfilesRefs.find(actionSelector.name);
            if (tablesIt != actionProfilesRefs.end()) {
                for (const auto& table : tablesIt->second) {
                    cstring tableName = prefix(blockPrefix, table);
                    profile.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), tableName));
                    selector.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), tableName));
                }
            }

            // We use the ActionSelector name for the action profile, and add a "_sel" suffix for
            // the action selector.
            cstring profileName = actionSelectorName;
            addP4InfoExternInstance(symbols, SymbolType::ACTION_PROFILE(), "ActionProfile",
                    profileName, actionSelector.annotations, profile,
                    p4Info);
            selector.set_action_profile_id(symbols.getId(
                        SymbolType::ACTION_PROFILE(), profileName));
            cstring selectorName = profileName + "_sel";
            addP4InfoExternInstance(symbols, SymbolType::ACTION_SELECTOR(), "ActionSelector",
                    selectorName, actionSelector.annotations,
                    selector, p4Info);
        }
    }

    /// calls @function on every extern method applied to the extern @object
    // TODO(Antonin): for some reason we sometimes get multiple calls on the
    // same extern method (but different IR node), and I haven't found out why
    // or when this happens.
    template <typename Func>
    void forAllExternMethodCalls(const IR::IDeclaration* object, Func function) {
        forAllMatching<IR::MethodCallExpression>(evaluatedProgram->getProgram(),
                                                 [&](const IR::MethodCallExpression* call) {
            auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (instance->is<P4::ExternMethod>() && instance->object == object) {
                function(instance->to<P4::ExternMethod>());
            }
        });
    }

    /// @return the table implementation property, or nullptr if the table has
    /// no such property.
    const IR::Property* getTableImplementationProperty(const IR::P4Table* table) {
        return table->properties->getProperty(implementationString);
    }

    const IR::IAnnotated* getTableImplementationAnnotations(
        const IR::P4Table* table, ReferenceMap* refMap) {
        auto impl = getTableImplementationProperty(table);
        if (impl == nullptr) return nullptr;
        if (!impl->value->template is<IR::ExpressionValue>()) return nullptr;
        auto expr = impl->value->template to<IR::ExpressionValue>()->expression;
        if (expr->template is<IR::ConstructorCallExpression>())
            return impl->template to<IR::IAnnotated>();
        if (expr->template is<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(expr->template to<IR::PathExpression>()->path, true);
            return decl->template to<IR::IAnnotated>();
        }
        return nullptr;
    }

    boost::optional<cstring> getTableImplementationName(
        const IR::P4Table* table, ReferenceMap* refMap) {
        auto impl = getTableImplementationProperty(table);
        if (impl == nullptr) return boost::none;
        if (!impl->value->template is<IR::ExpressionValue>()) {
            ::error("Expected %1% property value for table %2% to be an expression: %2%",
                    implementationString, table->controlPlaneName(), impl);
            return boost::none;
        }
        auto expr = impl->value->template to<IR::ExpressionValue>()->expression;
        if (expr->template is<IR::ConstructorCallExpression>()) return impl->controlPlaneName();
        if (expr->template is<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(expr->template to<IR::PathExpression>()->path, true);
            return decl->controlPlaneName();
        }
        return boost::none;
    }

    ReferenceMap* refMap;
    TypeMap* typeMap;
    const IR::ToplevelBlock* evaluatedProgram;

    /// Maps each action profile / selector to the set of tables referencing it.
    std::unordered_map<cstring, std::set<cstring> > actionProfilesRefs;
    /// The set of color-aware meters in the program.
    std::unordered_set<cstring> colorAwareMeters;

    cstring implementationString;
    cstring defaultPipeName = "pipe";

    static constexpr int64_t defaultMaxGroupSize = 120;
};

/// Implements @ref BFRuntimeArchHandlerIface for the Tofino architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize Tofino-specific symbols which are exposed to the control-plane.
class BFRuntimeArchHandlerTofino final : public BFN::BFRuntimeArchHandlerCommon<Arch::TNA> {
 public:
    template <typename Func>
    void forAllPipeBlocks(const IR::ToplevelBlock* evaluatedProgram, Func function) {
        auto main = evaluatedProgram->getMain();
        if (!main)
            ::fatal_error("Program does not contain a `main` module");
        auto cparams = main->getConstructorParameters();
        int index = -1;
        for (auto param : main->constantValue) {
            index++;
            if (!param.second) continue;
            auto pipe = param.second;
            if (!pipe->is<IR::PackageBlock>()) {
                ::error(ErrorType::ERR_INVALID, "%1% package block. You are compiling for the %2% "
                        "P4 architecture.\n"
                        "Please verify that you included the correct architecture file.",
                        pipe, BackendOptions().arch);
                return;
            }
            auto idxParam = cparams->getParameter(index);
            auto pipeName = idxParam->name;
            function(pipeName, pipe->to<IR::PackageBlock>());
        }
    }

    template <typename Func>
    void forAllPortMetadataBlocks(const IR::ToplevelBlock* evaluatedProgram, Func function) {
        auto main = evaluatedProgram->getMain();
        if (!main)
            ::fatal_error("Program does not contain a `main` module");
        if (main->type->name == "MultiParserSwitch") {
            int numParsersPerPipe = Device::numParsersPerPipe();
            auto parsersName = "ig_prsr";
            forAllPipeBlocks(evaluatedProgram, [&](cstring, const IR::PackageBlock* pkg) {
                auto parsers = pkg->findParameterValue(parsersName);
                BUG_CHECK(parsers, "Expected Block");
                if (!parsers->is<IR::PackageBlock>()) {
                    ::error(ErrorType::ERR_INVALID, "%1% package block. "
                            "You are compiling for the %2% P4 architecture.\n"
                            "Please verify that you included the correct architecture file.",
                            parsers, BackendOptions().arch);
                    return;
                }
                auto parsersBlock = parsers->to<IR::PackageBlock>();
                for (int idx = 0; idx < numParsersPerPipe; idx++) {
                    auto mpParserName = "prsr" + std::to_string(idx);
                    auto parser = parsersBlock->findParameterValue(mpParserName);
                    if (!parser) break;  // all parsers optional except for first one
                    auto parserBlock = parser->to<IR::ParserBlock>();
                    if (hasUserPortMetadata.count(parserBlock) == 0) {  // no extern, add default
                        auto portMetadataFullName =
                            getFullyQualifiedName(parserBlock, PortMetadata::name(), isMultiParser);
                        function(portMetadataFullName, parserBlock);
                    }
                }
            });
        } else {
            auto parserName = "ingress_parser";
            forAllPipeBlocks(evaluatedProgram, [&](cstring, const IR::PackageBlock* pkg) {
                auto *block = pkg->findParameterValue(parserName);
                if (!block) return;
                if (!block->is<IR::ParserBlock>()) return;
                auto parserBlock = block->to<IR::ParserBlock>();
                if (hasUserPortMetadata.count(parserBlock) == 0) {  // no extern, add default
                    auto portMetadataFullName =
                        getFullyQualifiedName(parserBlock, PortMetadata::name());
                    function(portMetadataFullName, parserBlock);
                }
            });
        }
    }

    BFRuntimeArchHandlerTofino(ReferenceMap* refMap,
                               TypeMap* typeMap,
                               const IR::ToplevelBlock* evaluatedProgram)
        : BFRuntimeArchHandlerCommon<Arch::TNA>(refMap, typeMap, evaluatedProgram) {
        implementationString = "implementation";

        std::set<cstring> pipes;
        // Create a map of all blocks to their pipe names. This map will
        // be used during collect and post processing to prefix
        // table/extern instances wherever applicable with a fully qualified
        // name. This distinction is necessary when the driver looks up
        // context.json across multiple pipes for the table name
        forAllPipeBlocks(evaluatedProgram, [&](cstring pipeName, const IR::PackageBlock* pkg) {
            Helpers::forAllEvaluatedBlocks(pkg, [&](const IR::Block* block) {
                auto decl = pkg->node->to<IR::Declaration_Instance>();
                cstring blockNamePrefix = pipeName;
                if (decl)
                    blockNamePrefix = decl->controlPlaneName();

                blockNamePrefixMap[block] = blockNamePrefix;
                pipes.insert(pipeName);
            });
        });

        // Update multi parser names
        static std::vector<cstring> gressNames = {"ig", "eg"};
        int numParsersPerPipe = Device::numParsersPerPipe();

        auto main = evaluatedProgram->getMain();
        if (!main)
            ::fatal_error("Program does not contain a `main` module");
        if (main->type->name == "MultiParserSwitch") {
            forAllPipeBlocks(evaluatedProgram, [&](cstring pipeName, const IR::PackageBlock* pkg) {
                BUG_CHECK(
                    pkg->type->name == "MultiParserPipeline",
                    "Only MultiParserPipeline pipes can be used with a MultiParserSwitch switch");
                for (auto gressName : gressNames) {
                    auto parsersName = gressName + "_prsr";
                    auto parsers = pkg->findParameterValue(parsersName);
                    BUG_CHECK(parsers && parsers->is<IR::PackageBlock>(), "Expected PackageBlock");
                    auto parsersBlock = parsers->to<IR::PackageBlock>();
                    auto decl = parsersBlock->node->to<IR::Declaration_Instance>();
                    if (decl)
                        parsersName = decl->controlPlaneName();
                    for (int idx = 0; idx < numParsersPerPipe; idx++) {
                        auto parserName = "prsr" + std::to_string(idx);
                        auto parser = parsersBlock->findParameterValue(parserName);
                        if (!parser) break;  // all parsers optional except for first one
                        BUG_CHECK(parser->is<IR::ParserBlock>(), "Expected ParserBlock");
                        auto parserBlock = parser->to<IR::ParserBlock>();

                        // Update Parser Block Names
                        auto parserFullName = pipeName + "." + parsersName + "." + parserName;
                        blockNamePrefixMap[parserBlock] = parserFullName;
                    }
                    isMultiParser = true;
                }
            });
        }
    }

    cstring getBlockNamePrefix(const IR::Block* blk) override {
        if (blockNamePrefixMap.count(blk) > 0)
            return blockNamePrefixMap[blk];
        return "";
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) override {
        collectExternInstanceCommon(symbols, externBlock);

        CHECK_NULL(externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        // Skip externs instantiated inside table declarations (as properties);
        // that should only apply to action profiles / selectors since direct
        // resources cannot be constructed in place for TNA.
        if (decl == nullptr) return;

        auto symName = getControlPlaneName(externBlock, decl);
        if (externBlock->type->name == "Lpf") {
            symbols->add(SymbolType::LPF(), symName);
        } else if (externBlock->type->name == "DirectLpf") {
            symbols->add(SymbolType::DIRECT_LPF(), symName);
        } else if (externBlock->type->name == "Wred") {
            symbols->add(SymbolType::WRED(), symName);
        } else if (externBlock->type->name == "DirectWred") {
            symbols->add(SymbolType::DIRECT_WRED(), symName);
        } else if (externBlock->type->name == "RegisterParam") {
            symbols->add(SymbolType::REGISTER_PARAM(), symName);
        }
    }

    void collectExternFunction(P4RuntimeSymbolTableIface*,
                               const P4::ExternFunction*) override {}

    void collectPortMetadataExternFunction(P4RuntimeSymbolTableIface* symbols,
                               const P4::ExternFunction* externFunction,
                               const IR::ParserBlock* parserBlock) {
        auto portMetadata = getPortMetadataExtract(externFunction, refMap, typeMap, nullptr);
        if (portMetadata) {
            if (hasUserPortMetadata.count(parserBlock)) {
                ::error("Cannot have multiple extern calls for %1%",
                        BFN::ExternPortMetadataUnpackString);
                return;
            }
            hasUserPortMetadata.insert(parserBlock);
            auto portMetadataFullName =
                getFullyQualifiedName(parserBlock, PortMetadata::name(), isMultiParser);
            symbols->add(SymbolType::PORT_METADATA(), portMetadataFullName);
        }
    }

    void collectParserSymbols(P4RuntimeSymbolTableIface* symbols,
                              const IR::ParserBlock* parserBlock) {
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);

        // Collect any extern functions it may invoke.
        for (auto state : parser->states) {
            forAllMatching<IR::MethodCallExpression>(state,
                          [&](const IR::MethodCallExpression* call) {
                auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                if (instance->is<P4::ExternFunction>())
                    collectPortMetadataExternFunction(symbols,
                            instance->to<P4::ExternFunction>(), parserBlock);
            });
        }

        for (auto s : parser->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                auto name = getFullyQualifiedName(parserBlock,
                                        inst->controlPlaneName(), true);
                symbols->add(SymbolType::VALUE_SET(), name);
            }
        }

        // Extract phase0 ingress intrinsic metadata name provided by user
        // (usually ig_intr_md). This value is prefixed to the key name in
        // phase0 table in bf-rt.json (e.g. ig_intr_md.ingress_port). TNA
        // translation will extract this value during midend and set the key
        // name in phase0 table in context.json to be consistent.
        auto *params = parser->getApplyParameters();
        for (auto p : *params) {
            if (p->type->toString() == "ingress_intrinsic_metadata_t") {
                BUG_CHECK(
                    ingressIntrinsicMdParamName.count(parserBlock) == 0 ||
                    strcmp(ingressIntrinsicMdParamName[parserBlock], p->name.toString()) == 0,
                    "%1%: Multiple names of intrinsic metadata found in this parser block",
                    parser->getName());
                ingressIntrinsicMdParamName[parserBlock] = p->name;
                break;
            }
        }
    }

    /// Holds information about each ingress and egress control flow in the P4
    /// program. This information is used to generate the Snapshot messages in
    /// P4Info.
    struct SnapshotInfo {
        cstring pipe;  /// one of "pipe0", "pipe1", "pipe2", "pipe3"
        cstring gress;  /// one of "ingress", "egress"
        size_t userHdrParamIdx;  /// the index of the "hdr" parameter in the control parameter list
        cstring name;  /// the control name as provided by the P4 program
        /// the list of fields to be exposed for snapshot for this control
        /// (including "POV bits"). Unlike previous struct fields, which are set
        /// by getSnapshotControls(), this vector is populated by
        /// collectSnapshot().
        std::set<SnapshotFieldInfo> fields;
    };

    /// Looks at all control objects relevant to snapshot (based on architecture
    /// knowledge) and populates the snapshotInfo map.
    void getSnapshotControls() {
        static std::vector<cstring> gressNames = {"ingress", "egress"};
        forAllPipeBlocks(evaluatedProgram, [&](cstring pipeName, const IR::PackageBlock* pkg) {
            for (auto gressName : gressNames) {
                auto gress = pkg->findParameterValue(gressName);
                BUG_CHECK(gress->is<IR::ControlBlock>(), "Expected control");
                auto control = gress->to<IR::ControlBlock>();
                if (blockNamePrefixMap.count(control) > 0)
                    pipeName = blockNamePrefixMap[control];
                snapshotInfo.emplace(control, SnapshotInfo{pipeName, gressName, 0u, "", {}});
            }
        });
    }

    /// This method is called for each control block. If a control is relevant
    /// for snapshot - based on the snapshotInfo map populated by
    /// getSnapshotControls() - then we inspect its parameters using the @ref
    /// SnapshotFieldFinder pass to build a list of snapshot fields. The caller
    /// is expected to pass the same @fieldIds table for every call to this
    /// method.
    void collectSnapshot(P4RuntimeSymbolTableIface* symbols,
                         const IR::ControlBlock* controlBlock,
                         SnapshotFieldIdTable* fieldIds) {
        CHECK_NULL(controlBlock);
        auto control = controlBlock->container;
        CHECK_NULL(control);
        auto sinfoIt = snapshotInfo.find(controlBlock);
        // if the block is not in snapshotInfo, it means it is not an ingress or
        // egress control.
        if (sinfoIt == snapshotInfo.end()) return;
        auto snapshot_name = control->externalName();
        // Collect unique snapshot names across pipes, this will ensure there is
        // only one snapshot field generated even if the field is replicated
        // across pipes. While setting a snapshot the user always provides a
        // pipe id so we do not need multiple snapshot entries.
        if (snapshots.count(snapshot_name)) {
            snapshotInfo.erase(sinfoIt);
            return;
        }
        snapshots.insert(snapshot_name);
        sinfoIt->second.name = snapshot_name;
        auto snapshotFields = &sinfoIt->second.fields;
        auto params = control->getApplyParameters();
        for (size_t idx = 0; idx < params->size(); idx++) {
            auto p = params->getParameter(idx);
            // include validity fields (POV bits) only for user-defined header
            // fields (the "hdr" parameter)
            auto includeValid = (idx == sinfoIt->second.userHdrParamIdx);
            SnapshotFieldFinder::find(
                typeMap, p->type, p->name, includeValid, snapshotFields, fieldIds);
        }
        symbols->add(SymbolType::SNAPSHOT(), snapshot_name);
    }

    /// For programs not using the MultiParserSwitch package, this method does
    /// not do antyhing. Otherwise, for each pipe and each gress within each
    /// pipe, it builds the list of possible parsers. For each parser, we store
    /// the "architecture name", the "P4 type name" and the "user-provided name"
    /// (if applicable, i.e. if the P4 programmer used a declaration).
    void collectParserChoices(P4RuntimeSymbolTableIface* symbols) {
        static std::vector<cstring> gressNames = {"ig", "eg"};
        int numParsersPerPipe = Device::numParsersPerPipe();

        auto main = evaluatedProgram->getMain();
        if (!main)
            ::fatal_error("Program does not contain a `main` module");
        if (main->type->name != "MultiParserSwitch") return;

        forAllPipeBlocks(evaluatedProgram, [&](cstring pipeName, const IR::PackageBlock* pkg) {
            BUG_CHECK(
                pkg->type->name == "MultiParserPipeline",
                "Only MultiParserPipeline pipes can be used with a MultiParserSwitch switch");
            for (auto gressName : gressNames) {
                ::barefoot::ParserChoices parserChoices;

                if (auto decl = pkg->node->to<IR::Declaration_Instance>())
                    pipeName = decl->Name();

                parserChoices.set_pipe(pipeName);
                if (gressName == "ig")
                    parserChoices.set_direction(::barefoot::DIRECTION_INGRESS);
                else
                    parserChoices.set_direction(::barefoot::DIRECTION_EGRESS);

                auto parsersName = gressName + "_prsr";
                auto parsers = pkg->findParameterValue(parsersName);
                BUG_CHECK(parsers && parsers->is<IR::PackageBlock>(), "Expected PackageBlock");
                auto parsersBlock = parsers->to<IR::PackageBlock>();
                auto decl = parsersBlock->node->to<IR::Declaration_Instance>();
                if (decl)
                    parsersName = decl->controlPlaneName();
                for (int idx = 0; idx < numParsersPerPipe; idx++) {
                    auto parserName = "prsr" + std::to_string(idx);
                    auto parser = parsersBlock->findParameterValue(parserName);
                    if (!parser) break;  // all parsers optional except for first one
                    BUG_CHECK(parser->is<IR::ParserBlock>(), "Expected ParserBlock");
                    auto parserBlock = parser->to<IR::ParserBlock>();
                    auto decl = parserBlock->node->to<IR::Declaration_Instance>();
                    auto userName = (decl == nullptr) ? "" : decl->controlPlaneName();

                    auto choice = parserChoices.add_choices();
                    auto parserFullName = prefix(parsersName, parserName);
                    parserFullName = prefix(pipeName, parserFullName);
                    choice->set_arch_name(parserFullName);
                    choice->set_type_name(parserBlock->getName().name);
                    choice->set_user_name(userName);
                }
                auto parsersFullName = prefix(pipeName, parsersName);
                symbols->add(SymbolType::PARSER_CHOICES(), parsersFullName);
                parserConfiguration.emplace(parsersFullName, parserChoices);
            }
        });
    }

    void collectExtra(P4RuntimeSymbolTableIface* symbols) override {
        // Collect value sets. This step is required because value set support
        // in "standard" P4Info is currently insufficient.
        // Also retrieve user-provided name for ig_intr_md parameter in ingress
        // parser to use as key name for phase0 table.
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ParserBlock>()) return;
            collectParserSymbols(symbols, block->to<IR::ParserBlock>());
        });

        // Collect snapshot fields for each control by populating the
        // snapshotInfo map.
        getSnapshotControls();
        SnapshotFieldIdTable snapshotFieldIds;
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ControlBlock>()) return;
            collectSnapshot(symbols, block->to<IR::ControlBlock>(), &snapshotFieldIds);
        });

        collectParserChoices(symbols);

        // Check if each parser block in program has a port metadata extern
        // defined, if not we add a default instances to symbol table
        forAllPortMetadataBlocks(evaluatedProgram, [&](cstring portMetadataFullName,
                                                       const IR::ParserBlock*) {
            symbols->add(SymbolType::PORT_METADATA(), portMetadataFullName);
        });
    }

    void postCollect(const P4RuntimeSymbolTableIface& symbols) override {
        (void)symbols;
        // analyze action profiles / selectors and build a mapping from action
        // profile / selector name to the set of tables referencing them
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::TableBlock>()) return;
            auto table = block->to<IR::TableBlock>()->container;
            auto implementation = getTableImplementationName(table, refMap);
            if (implementation)
                actionProfilesRefs[*implementation].insert(table->controlPlaneName());
        });

        // analyze action profile used by action selector and adds the set of
        // table referenced by action selector to the action profile.
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ExternBlock>()) return;
            auto selectorExternBlock = block->to<IR::ExternBlock>();
            if (selectorExternBlock->type->name != "ActionSelector")
                return;
            auto selectorDecl = selectorExternBlock->node->to<IR::Declaration_Instance>();
            CHECK_NULL(selectorDecl);
            auto profile = selectorExternBlock->getParameterValue("action_profile");
            if (profile) {
                auto profileExternBlock = profile->to<IR::ExternBlock>();
                auto profileDecl = profileExternBlock->node->to<IR::Declaration_Instance>();
                CHECK_NULL(profileDecl);
                actionProfilesRefs[profileDecl->controlPlaneName()].insert(
                        actionProfilesRefs[selectorDecl->controlPlaneName()].begin(),
                        actionProfilesRefs[selectorDecl->controlPlaneName()].end());
            }
        });

        // Creates a set of color-aware meters by inspecting every call to the
        // execute method on each meter instance: if at least one method call
        // includes a second argument (pre-color), then the meter is
        // color-aware.
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ExternBlock>()) return;
            auto externBlock = block->to<IR::ExternBlock>();
            if (externBlock->type->name != "Meter" && externBlock->type->name != "DirectMeter")
                return;
            auto decl = externBlock->node->to<IR::Declaration_Instance>();
            // Should not happen for TNA: direct meters cannot be constructed
            // in-place.
            CHECK_NULL(decl);
            forAllExternMethodCalls(decl, [&](const P4::ExternMethod* method) {
                auto call = method->expr;
                if (call->arguments->size() == 2) {
                    LOG3("Meter " << decl->controlPlaneName() << " is color-aware "
                         << "because of 2-argument call to execute()");
                    colorAwareMeters.insert(decl->controlPlaneName());
                }
            });
        });

        class RegisterParamFinder : public Inspector {
            BFRuntimeArchHandlerTofino &self;

            bool checkExtern(const IR::Declaration_Instance *decl, cstring name) {
                if (auto *specialized = decl->type->to<IR::Type_Specialized>())
                    if (auto *name_type = specialized->baseType->to<IR::Type_Name>())
                        return name_type->path->name == name;
                return false;
            }

         public:
            RegisterParamFinder(BFRuntimeArchHandlerTofino &self) : self(self) {}
            bool preorder(const IR::MethodCallExpression *mce) override {
                auto method = P4::MethodInstance::resolve(mce, self.refMap, self.typeMap);
                CHECK_NULL(method);
                if (method->object == nullptr)  // extern functions
                    return true;
                auto *reg_param = method->object->to<IR::Declaration_Instance>();
                if (reg_param == nullptr)  // packet_in packet
                    return true;
                if (!checkExtern(reg_param, "RegisterParam"))
                    return true;
                // Find the declaration of RegisterAction
                auto *reg_action = findContext<IR::Declaration_Instance>();
                if (reg_action == nullptr)
                    return true;
                if (!checkExtern(reg_action, "DirectRegisterAction")
                        && !checkExtern(reg_action, "RegisterAction"))
                    return true;
                BUG_CHECK(reg_action->arguments->size() > 0, "%1%: Missing argument", reg_action);
                auto *rap = reg_action->arguments->at(0)->expression->to<IR::PathExpression>();
                if (rap == nullptr)
                    return true;
                // Find the declaration of Register or DirectRegister
                auto *rap_decl = getDeclInst(self.refMap, rap);
                if (rap_decl == nullptr)
                    return true;
                if (checkExtern(rap_decl, "Register"))
                    self.registerParam2register[reg_param->controlPlaneName()]
                        = rap_decl->controlPlaneName();
                if (checkExtern(rap_decl, "DirectRegister"))
                    self.registerParam2table[reg_param->controlPlaneName()]
                        = rap_decl->controlPlaneName();
                return true;
            }
            void postorder(const IR::P4Table *table) override {
                // Find the M/A table with attached direct register that uses a register parameter
                auto *registers = table->properties->getProperty("registers");
                if (registers == nullptr)
                    return;
                auto *registers_value = registers->value->to<IR::ExpressionValue>();
                CHECK_NULL(registers_value);
                auto *registers_path = registers_value->expression->to<IR::PathExpression>();
                CHECK_NULL(registers_path);
                auto *registers_decl = getDeclInst(self.refMap, registers_path);
                CHECK_NULL(registers_decl);
                // Replace a direct register with a M/A table it is attached to
                for (auto r : self.registerParam2table)
                    if (r.second == registers_decl->controlPlaneName())
                        self.registerParam2table[r.first] = table->controlPlaneName();
            }
        };

        evaluatedProgram->getProgram()->apply(RegisterParamFinder(*this));
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) override {
        CHECK_NULL(tableBlock);
        auto tableDeclaration = tableBlock->container;
        auto blockPrefix = blockNamePrefixMap[tableBlock];

        addTablePropertiesCommon(symbols, p4info, table, tableBlock, blockPrefix);

        auto directLpf = getDirectLpf(tableDeclaration, refMap, typeMap);
        if (directLpf) {
            auto id = symbols.getId(SymbolType::DIRECT_LPF(),
                    prefix(blockPrefix, directLpf->name));
            table->add_direct_resource_ids(id);
            addLpf(symbols, p4info, *directLpf, blockPrefix);
        }

        auto directWred = getDirectWred(tableDeclaration, refMap, typeMap);
        if (directWred) {
            auto id = symbols.getId(SymbolType::DIRECT_WRED(),
                    prefix(blockPrefix, directWred->name));
            table->add_direct_resource_ids(id);
            addWred(symbols, p4info, *directWred, blockPrefix);
        }
    }


    /// @return the direct filter instance associated with @table, if it has
    /// one, or boost::none otherwise. Used as a helper for getDirectLpf and
    /// getDirectWred.
    template <typename T>
    static boost::optional<T> getDirectFilter(const IR::P4Table *table,
                                              ReferenceMap* refMap,
                                              TypeMap* typeMap,
                                              cstring filterType) {
        auto directFilterInstance = Helpers::getExternInstanceFromProperty(
            table, "filters", refMap, typeMap);
        if (!directFilterInstance) return boost::none;
        CHECK_NULL(directFilterInstance->type);
        if (directFilterInstance->type->name != filterType) return boost::none;
        return T::fromDirect(*directFilterInstance, table);
    }

    /// @return the direct Lpf instance associated with @table, if it has one,
    /// or boost::none otherwise.
    static boost::optional<Lpf> getDirectLpf(const IR::P4Table *table,
                                             ReferenceMap* refMap,
                                             TypeMap* typeMap) {
        return getDirectFilter<Lpf>(table, refMap, typeMap, "DirectLpf");
    }

    /// @return the direct Wred instance associated with @table, if it has one,
    /// or boost::none otherwise.
    static boost::optional<Wred> getDirectWred(const IR::P4Table *table,
                                               ReferenceMap* refMap,
                                               TypeMap* typeMap) {
        return getDirectFilter<Wred>(table, refMap, typeMap, "DirectWred");
    }

    void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock) override {
        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        // Skip externs instantiated inside table declarations (constructed in
        // place as properties).
        if (decl == nullptr) return;

        using Helpers::Counterlike;
        using Helpers::CounterlikeTraits;

        auto pipeName = blockNamePrefixMap[externBlock];

        addExternInstanceCommon(symbols, p4info, externBlock, pipeName);

        auto p4RtTypeInfo = p4info->mutable_type_info();
        // Direct resources are handled by addTableProperties.
        if (externBlock->type->name == "Lpf") {
            auto lpf = Lpf::from(externBlock);
            if (lpf) addLpf(symbols, p4info, *lpf, pipeName);
        } else if (externBlock->type->name == "Wred") {
            auto wred = Wred::from(externBlock);
            if (wred) addWred(symbols, p4info, *wred, pipeName);
        } else if (externBlock->type->name == "RegisterParam") {
            auto register_param_ = RegisterParam::from(externBlock, refMap, typeMap, p4RtTypeInfo);
            if (register_param_) addRegisterParam(symbols, p4info, *register_param_, pipeName);
        }
    }

    void addExternFunction(const P4RuntimeSymbolTableIface&,
                           p4configv1::P4Info*, const P4::ExternFunction*) override {}

    void addPortMetadataExternFunction(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const P4::ExternFunction* externFunction,
                           const IR::ParserBlock* parserBlock) {
        auto p4RtTypeInfo = p4info->mutable_type_info();
        auto portMetadata = getPortMetadataExtract(externFunction, refMap, typeMap, p4RtTypeInfo);
        if (portMetadata) {
            if (blockNamePrefixMap.count(parserBlock)) {
                auto portMetadataFullName =
                    getFullyQualifiedName(parserBlock, PortMetadata::name(), isMultiParser);
                addPortMetadata(symbols, p4info, *portMetadata, portMetadataFullName, parserBlock);
            }
        }
    }

    void analyzeParser(const P4RuntimeSymbolTableIface& symbols,
                       ::p4::config::v1::P4Info* p4info,
                       const IR::ParserBlock* parserBlock) {
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);

        for (auto s : parser->parserLocals) {
            if (auto inst = s->to<IR::P4ValueSet>()) {
                auto namePrefix =
                    getFullyQualifiedName(parserBlock, inst->controlPlaneName(), true);
                auto valueSet = ValueSet::from(namePrefix, inst, refMap,
                        typeMap, p4info->mutable_type_info());
                if (valueSet) addValueSet(symbols, p4info, *valueSet);
            }
        }

        // Add any extern functions within parser states.
        for (auto state : parser->states) {
            forAllMatching<IR::MethodCallExpression>(state,
                          [&](const IR::MethodCallExpression* call) {
                auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                if (instance->is<P4::ExternFunction>())
                    addPortMetadataExternFunction(symbols, p4info,
                            instance->to<P4::ExternFunction>(), parserBlock);
            });
        }
    }

    void postAdd(const P4RuntimeSymbolTableIface& symbols,
                 ::p4::config::v1::P4Info* p4info) override {
        // Generates Tofino-specific ValueSet P4Info messages. This step is
        // required because value set support in "standard" P4Info is currently
        // insufficient: the standard ValueSet message restricts the element
        // type to a simple binary string (P4Runtime v1.0 limitation).
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ParserBlock>()) return;
            analyzeParser(symbols, p4info, block->to<IR::ParserBlock>());
        });

        // Check if each parser block in program has a port metadata extern
        // defined, if not we add a default instances
        forAllPortMetadataBlocks(evaluatedProgram, [&](cstring portMetadataFullName,
                                                       const IR::ParserBlock* parserBlock) {
            addPortMetadataDefault(symbols, p4info, portMetadataFullName, parserBlock);
        });

        for (const auto& snapshot : snapshotInfo)
            addSnapshot(symbols, p4info, snapshot.second);

        for (const auto& parser : parserConfiguration)
            addParserChoices(symbols, p4info, parser.first, parser.second);
    }

    /// @return serialization information for the port_metadata_extract() call represented by
    /// @call, or boost::none if @call is not a port_metadata_extract() call or is invalid.
    static boost::optional<PortMetadata>
    getPortMetadataExtract(const P4::ExternFunction* function,
                           ReferenceMap* refMap,
                           const TypeMap* typeMap,
                           p4configv1::P4TypeInfo* p4RtTypeInfo) {
        if (function->method->name != BFN::ExternPortMetadataUnpackString)
            return boost::none;

        if (auto *call = function->expr->to<IR::MethodCallExpression>()) {
            auto *typeArg = call->typeArguments->at(0);
            auto typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeArg, p4RtTypeInfo);
            return PortMetadata{typeSpec};
        }
        return boost::none;
    }


    void addPortMetadata(const P4RuntimeSymbolTableIface& symbols,
                         p4configv1::P4Info* p4Info,
                         const PortMetadata& portMetadataExtract,
                         const cstring& name,
                         const IR::ParserBlock* parserBlock) {
        ::barefoot::PortMetadata portMetadata;
        portMetadata.mutable_type_spec()->CopyFrom(*portMetadataExtract.typeSpec);
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);
        BUG_CHECK(ingressIntrinsicMdParamName.count(parserBlock),
                  "%1%: Name of the intrinsic metadata not found for this parser block",
                  parser->getName());
        portMetadata.set_key_name(ingressIntrinsicMdParamName[parserBlock]);
        addP4InfoExternInstance(
            symbols, SymbolType::PORT_METADATA(), "PortMetadata",
            name, nullptr, portMetadata, p4Info);
    }

    void addPortMetadataDefault(const P4RuntimeSymbolTableIface& symbols,
                                p4configv1::P4Info* p4Info, const cstring& name,
                                const IR::ParserBlock* parserBlock) {
        ::barefoot::PortMetadata portMetadata;
        CHECK_NULL(parserBlock);
        auto parser = parserBlock->container;
        CHECK_NULL(parser);
        BUG_CHECK(ingressIntrinsicMdParamName.count(parserBlock),
                  "%1%: Name of the intrinsic metadata not found for this parser block",
                  parser->getName());
        portMetadata.set_key_name(ingressIntrinsicMdParamName[parserBlock]);
        addP4InfoExternInstance(
            symbols, SymbolType::PORT_METADATA(), "PortMetadata",
            name, nullptr, portMetadata, p4Info);
    }

    void addSnapshot(const P4RuntimeSymbolTableIface& symbols,
                     p4configv1::P4Info* p4Info,
                     const SnapshotInfo& snapshotInstance) {
        ::barefoot::Snapshot snapshot;
        snapshot.set_pipe(snapshotInstance.pipe);
        if (snapshotInstance.gress == "ingress")
            snapshot.set_direction(::barefoot::DIRECTION_INGRESS);
        else if (snapshotInstance.gress == "egress")
            snapshot.set_direction(::barefoot::DIRECTION_EGRESS);
        else
            BUG("Invalid gress '%1%'", snapshotInstance.gress);
        for (const auto& f : snapshotInstance.fields) {
            auto newF = snapshot.add_fields();
            newF->set_id(f.id);
            newF->set_name(f.name);
            newF->set_bitwidth(f.bitwidth);
        }
        addP4InfoExternInstance(
            symbols, SymbolType::SNAPSHOT(), "Snapshot",
            snapshotInstance.name, nullptr, snapshot, p4Info);
    }

    void addParserChoices(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          cstring name,
                          const ::barefoot::ParserChoices& parserChoices) {
        addP4InfoExternInstance(
            symbols, SymbolType::PARSER_CHOICES(), "ParserChoices",
            name, nullptr, parserChoices, p4Info);
    }

    void addLpf(const P4RuntimeSymbolTableIface& symbols,
                p4configv1::P4Info* p4Info,
                const Lpf& lpfInstance,
                const cstring pipeName) {
        auto lpfName = prefix(pipeName, lpfInstance.name);
        if (lpfInstance.table) {
            ::barefoot::DirectLpf lpf;
            auto tableName = prefix(pipeName, *lpfInstance.table);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), tableName);
            lpf.set_direct_table_id(tableId);
            addP4InfoExternInstance(
                symbols, SymbolType::DIRECT_LPF(), "DirectLpf",
                lpfName, lpfInstance.annotations, lpf,
                p4Info);
        } else {
            ::barefoot::Lpf lpf;
            lpf.set_size(lpfInstance.size);
            addP4InfoExternInstance(
                symbols, SymbolType::LPF(), "Lpf",
                lpfName, lpfInstance.annotations, lpf,
                p4Info);
        }
    }

    /// Set common fields between barefoot::Wred and barefoot::DirectWred.
    template <typename Kind>
    void setWredCommon(Kind *wred, const Wred& wredInstance) {
        using ::barefoot::WredSpec;
        auto wred_spec = wred->mutable_spec();
        wred_spec->set_drop_value(wredInstance.dropValue);
        wred_spec->set_no_drop_value(wredInstance.noDropValue);
    }

    void addWred(const P4RuntimeSymbolTableIface& symbols,
                 p4configv1::P4Info* p4Info,
                 const Wred& wredInstance,
                 const cstring pipeName) {
        auto wredName = prefix(pipeName, wredInstance.name);
        if (wredInstance.table) {
            ::barefoot::DirectWred wred;
            setWredCommon(&wred, wredInstance);
            auto tableName = prefix(pipeName, *wredInstance.table);
            auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), tableName);
            wred.set_direct_table_id(tableId);
            addP4InfoExternInstance(
                symbols, SymbolType::DIRECT_WRED(), "DirectWred",
                wredName, wredInstance.annotations, wred,
                p4Info);
        } else {
            ::barefoot::Wred wred;
            setWredCommon(&wred, wredInstance);
            wred.set_size(wredInstance.size);
            addP4InfoExternInstance(
                symbols, SymbolType::WRED(), "Wred",
                wredName, wredInstance.annotations, wred,
                p4Info);
        }
    }

    // For RegisterParams, the table name should have the associated pipe prefix but
    // the data field names should not since they have local scope.
    void addRegisterParam(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          const RegisterParam& registerParamInstance,
                          cstring pipeName = "") {
        p4rt_id_t tableId = 0;
        if (registerParam2register.count(registerParamInstance.name) > 0)
            tableId = symbols.getId(SymbolType::REGISTER(),
                prefix(pipeName, registerParam2register[registerParamInstance.name]));
        else if (registerParam2table.count(registerParamInstance.name) > 0)
            tableId = symbols.getId(P4RuntimeSymbolType::TABLE(),
                prefix(pipeName, registerParam2table[registerParamInstance.name]));
        // If a register parameter is not used, it will show up in neither of the maps.
        ::barefoot::RegisterParam register_param_;
        register_param_.mutable_type_spec()->CopyFrom(*registerParamInstance.typeSpec);
        register_param_.set_table_id(tableId);
        register_param_.set_initial_value(registerParamInstance.initial_value);
        register_param_.set_data_field_name(registerParamInstance.name);
        auto registerParamName = prefix(pipeName, registerParamInstance.name);
        addP4InfoExternInstance(
            symbols, SymbolType::REGISTER_PARAM(), "RegisterParam",
            registerParamName, registerParamInstance.annotations, register_param_,
            p4Info);
    }

 private:
    /// The set of snapshots in the program.
    std::unordered_set<cstring> snapshots;
    /// A set of all all (parser) blocks containing port metadata
    std::unordered_set<const IR::Block *> hasUserPortMetadata;

    /// This is the user defined value for ingress_intrinsic_metadata_t as
    /// specified in the P4 program (can be different for each pipe/parser)
    std::unordered_map<const IR::ParserBlock *, cstring> ingressIntrinsicMdParamName;

    using SnapshotInfoMap = std::map<const IR::ControlBlock*, SnapshotInfo>;
    SnapshotInfoMap snapshotInfo;

    std::unordered_map<const IR::Block *, cstring> blockNamePrefixMap;

    std::map<cstring, ::barefoot::ParserChoices> parserConfiguration;

    bool isMultiParser;

    /// Maps a register parameter to a register (indirect registers)
    std::unordered_map<cstring, cstring> registerParam2register;
    /// Maps a register parameter to a match table (direct registers)
    std::unordered_map<cstring, cstring> registerParam2table;
};

/// Implements @ref BFRuntimeArchHandlerCommon interface for PSA architectures. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize PSA-specific symbols which are exposed to the control-plane.
class BFRuntimeArchHandlerPSA final : public BFRuntimeArchHandlerCommon<Arch::PSA> {
 public:
    BFRuntimeArchHandlerPSA(ReferenceMap* refMap,
                               TypeMap* typeMap,
                               const IR::ToplevelBlock* evaluatedProgram)
        : BFRuntimeArchHandlerCommon<Arch::PSA>(refMap, typeMap, evaluatedProgram) {
        implementationString = "psa_implementation";
    }

    void postCollect(const P4RuntimeSymbolTableIface& symbols) {
        (void)symbols;
        // analyze action profiles / selectors and build a mapping from action
        // profile / selector name to the set of tables referencing them
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::TableBlock>()) return;
            auto table = block->to<IR::TableBlock>()->container;
            auto implementation = getTableImplementationName(table, refMap);
            if (implementation)
                actionProfilesRefs[*implementation].insert(table->controlPlaneName());
        });

        // Creates a set of color-aware meters by inspecting every call to the
        // execute method on each meter instance: if at least one method call
        // includes a second argument (pre-color), then the meter is
        // color-aware.
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (!block->is<IR::ExternBlock>()) return;
            auto externBlock = block->to<IR::ExternBlock>();
            if (externBlock->type->name != "Meter" && externBlock->type->name != "DirectMeter")
                return;
            auto decl = externBlock->node->to<IR::Declaration_Instance>();
            // Should not happen for TNA: direct meters cannot be constructed
            // in-place.
            CHECK_NULL(decl);
            forAllExternMethodCalls(decl, [&](const P4::ExternMethod* method) {
                auto call = method->expr;
                if (call->arguments->size() == 2) {
                    LOG3("Meter " << decl->controlPlaneName() << " is color-aware "
                         << "because of 2-argument call to execute()");
                    colorAwareMeters.insert(decl->controlPlaneName());
                }
            });
        });
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) {
        CHECK_NULL(tableBlock);
        addTablePropertiesCommon(symbols, p4info, table, tableBlock, defaultPipeName);
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) override {
        collectExternInstanceCommon(symbols, externBlock);
    }

    void collectExternFunction(P4RuntimeSymbolTableIface*,
                               const P4::ExternFunction*) {}


    void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock) {
        addExternInstanceCommon(symbols, p4info, externBlock, defaultPipeName);
    }

    void addExternFunction(const P4RuntimeSymbolTableIface&,
                           p4configv1::P4Info*, const P4::ExternFunction*) {
    }
};

/// The architecture handler builder implementation for Tofino.
struct TofinoArchHandlerBuilder : public P4::ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface {
    P4::ControlPlaneAPI::P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override {
        return new BFRuntimeArchHandlerTofino(refMap, typeMap, evaluatedProgram);
    }
};

/// The architecture handler builder implementation for PSA.
struct PSAArchHandlerBuilder : public P4::ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface {
    P4::ControlPlaneAPI::P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override {
        return new BFRuntimeArchHandlerPSA(refMap, typeMap, evaluatedProgram);
    }
};

}  // namespace BFN
#endif  /* DPDK_CONTROL_PLANE_BFRUNTIME_ARCH_HANDLER_H_ */
