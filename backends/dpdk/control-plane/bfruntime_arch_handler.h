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
#include "control-plane/p4RuntimeArchStandard.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "control-plane/typeSpecConverter.h"

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"

#include "midend/eliminateTypedefs.h"

#include "p4/config/dpdk/p4info.pb.h"

using P4::ReferenceMap;
using P4::TypeMap;
using P4::ControlPlaneAPI::Helpers::getExternInstanceFromProperty;
using P4::ControlPlaneAPI::Helpers::isExternPropertyConstructedInPlace;

namespace p4configv1 = ::p4::config::v1;

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

/// Declarations specific to standard architectures (v1model & PSA).
namespace Standard {

/// Extends P4RuntimeSymbolType for the DPDK extern types.
class SymbolTypeDPDK final : public SymbolType {
 public:
    SymbolTypeDPDK() = delete;

    static P4RuntimeSymbolType ACTION_SELECTOR() {
        return P4RuntimeSymbolType::make(dpdk::P4Ids::ACTION_SELECTOR);
    }
};

/// The information about an action profile which is necessary to generate its
/// serialized representation.
struct ActionSelector {
    const cstring name;  // The fully qualified external name of this action selector.
    const int64_t size;  // TODO(hanw): size does not make sense with new ActionSelector P4 extern
    const int64_t maxGroupSize;
    const int64_t numGroups;
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // profile declaration.

    static constexpr int64_t defaultMaxGroupSize = 120;

    p4rt_id_t getId(const P4RuntimeSymbolTableIface& symbols) const {
        return symbols.getId(SymbolTypeDPDK::ACTION_SELECTOR(), name + "_sel");
    }
};

class BFRuntimeArchHandlerPSA final : public P4RuntimeArchHandlerCommon<Arch::PSA> {
 public:
    using ArchCounterExtern = CounterExtern<Arch::PSA>;
    using CounterTraits = Helpers::CounterlikeTraits<ArchCounterExtern>;
    using ArchMeterExtern = MeterExtern<Arch::PSA>;
    using MeterTraits = Helpers::CounterlikeTraits<ArchMeterExtern>;

    using Counter = p4configv1::Counter;
    using Meter = p4configv1::Meter;
    using CounterSpec = p4configv1::CounterSpec;
    using MeterSpec = p4configv1::MeterSpec;

    BFRuntimeArchHandlerPSA(ReferenceMap* refMap, TypeMap* typeMap,
                            const IR::ToplevelBlock* evaluatedProgram)
        : P4RuntimeArchHandlerCommon<Arch::PSA>(refMap, typeMap, evaluatedProgram) { }

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

    boost::optional<ActionSelector>
    getActionSelector(const IR::ExternBlock* instance) {
        auto actionSelDecl = instance->node->to<IR::IDeclaration>();
        // to be deleted, used to support deprecated ActionSelector constructor.
        auto size = instance->getParameterValue("size");
        BUG_CHECK(size->is<IR::Constant>(), "Non-constant size");
        return ActionSelector{actionSelDecl->controlPlaneName(),
                              size->to<IR::Constant>()->asInt(),
                              ActionSelector::defaultMaxGroupSize,
                              size->to<IR::Constant>()->asInt(),
                              actionSelDecl->to<IR::IAnnotated>()};
    }

    void addActionSelector(const P4RuntimeSymbolTableIface& symbols,
                          p4configv1::P4Info* p4Info,
                          const ActionSelector& actionSelector) {
        ::dpdk::ActionSelector selector;
        selector.set_max_group_size(actionSelector.maxGroupSize);
        selector.set_num_groups(actionSelector.numGroups);
        p4configv1::ActionProfile profile;
        profile.set_size(actionSelector.size);
        auto tablesIt = actionProfilesRefs.find(actionSelector.name);
        if (tablesIt != actionProfilesRefs.end()) {
            for (const auto& table : tablesIt->second) {
                profile.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), table));
                selector.add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE(), table));
            }
        }
        // We use the ActionSelector name for the action profile, and add a "_sel" suffix for
        // the action selector.
        cstring profileName = actionSelector.name;
        selector.set_action_profile_id(symbols.getId(
                    SymbolType::ACTION_PROFILE(), profileName));
        cstring selectorName = profileName + "_sel";
        addP4InfoExternInstance(symbols, SymbolTypeDPDK::ACTION_SELECTOR(),
                "ActionSelector", selectorName, actionSelector.annotations,
                selector, p4Info);
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) {
        P4RuntimeArchHandlerCommon<Arch::PSA>::collectExternInstance(symbols, externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        if (decl == nullptr) return;
        if (externBlock->type->name == "Digest") {
            symbols->add(SymbolType::DIGEST(), decl);
         } else if (externBlock->type->name == ActionSelectorTraits<Arch::PSA>::typeName()) {
            auto selName = decl->controlPlaneName() + "_sel";
            auto profName = decl->controlPlaneName();
            symbols->add(SymbolTypeDPDK::ACTION_SELECTOR(), selName);
            symbols->add(SymbolType::ACTION_PROFILE(), profName);
        }
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) {
        P4RuntimeArchHandlerCommon<Arch::PSA>::addTableProperties(
            symbols, p4info, table, tableBlock);

        auto tableDeclaration = tableBlock->container;
        bool supportsTimeout = getSupportsTimeout(tableDeclaration);
        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const IR::ExternBlock* externBlock) override {
        P4RuntimeArchHandlerCommon<Arch::PSA>::addExternInstance(
            symbols, p4info, externBlock);

        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        if (decl == nullptr) return;
        auto p4RtTypeInfo = p4info->mutable_type_info();
        if (externBlock->type->name == "Digest") {
            auto digest = getDigest(decl, p4RtTypeInfo);
            if (digest) addDigest(symbols, p4info, *digest);
        } else if (externBlock->type->name == "ActionSelector") {
            auto actionSelector = getActionSelector(externBlock);
            if (actionSelector) addActionSelector(symbols, p4info, *actionSelector);
        }
    }

    /// @return serialization information for the Digest extern instacne @decl
    boost::optional<Digest> getDigest(const IR::Declaration_Instance* decl,
                                      p4configv1::P4TypeInfo* p4RtTypeInfo) {
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

    /// @return true if @table's 'psa_idle_timeout' property exists and is true. This
    /// indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table* table) {
        auto timeout = table->properties->getProperty("psa_idle_timeout");

        if (timeout == nullptr) return false;

        if (auto exprValue = timeout->value->to<IR::ExpressionValue>()) {
            if (auto expr = exprValue->expression) {
                if (auto member = expr->to<IR::Member>()) {
                    if (member->member == "NOTIFY_CONTROL") {
                        return true;
                    } else if (member->member == "NO_TIMEOUT") {
                        return false;
                    }
                } else if (expr->is<IR::PathExpression>()) {
                    ::error(ErrorType::ERR_UNEXPECTED,
                        "Unresolved value %1% for psa_idle_timeout "
                        "property on table %2%. Must be a constant and one of "
                        "{ NOTIFY_CONTROL, NO_TIMEOUT }", timeout, table);
                    return false;
                }
            }
        }

        ::error(ErrorType::ERR_UNEXPECTED,
                "Unexpected value %1% for psa_idle_timeout "
                "property on table %2%. Supported values are "
                "{ NOTIFY_CONTROL, NO_TIMEOUT }", timeout, table);
        return false;
    }
};

/// The architecture handler builder implementation for PSA.
struct PSAArchHandlerBuilderForDPDK : public P4::ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface {
    P4::ControlPlaneAPI::P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override {
        return new P4::ControlPlaneAPI::Standard::BFRuntimeArchHandlerPSA(refMap, typeMap, evaluatedProgram);
    }
};

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4

#endif  /* DPDK_CONTROL_PLANE_BFRUNTIME_ARCH_HANDLER_H_ */
