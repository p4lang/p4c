/*
Copyright 2018-present Barefoot Networks, Inc.

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

#include <set>
#include <unordered_map>

#include <boost/optional.hpp>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "typeSpecConverter.h"

#include "p4RuntimeArchHandler.h"
#include "p4RuntimeArchStandard.h"

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

namespace Standard {

/// Implements @ref P4RuntimeArchHandlerIface for the v1model architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize v1model-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerV1Model final : public P4RuntimeArchHandlerCommon<Arch::V1MODEL> {
 public:
    P4RuntimeArchHandlerV1Model(ReferenceMap* refMap,
                                TypeMap* typeMap,
                                const IR::ToplevelBlock* evaluatedProgram)
        : P4RuntimeArchHandlerCommon<Arch::V1MODEL>(refMap, typeMap, evaluatedProgram) { }

    void collectExternFunction(P4RuntimeSymbolTableIface* symbols,
                               const P4::ExternFunction* externFunction) override {
        auto digest = getDigestCall(externFunction, refMap, typeMap, nullptr);
        if (digest) symbols->add(SymbolType::DIGEST(), digest->name);
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) override {
        P4RuntimeArchHandlerCommon<Arch::V1MODEL>::addTableProperties(
            symbols, p4info, table, tableBlock);
        auto tableDeclaration = tableBlock->container;

        bool supportsTimeout = getSupportsTimeout(tableDeclaration);
        if (supportsTimeout) {
            table->set_idle_timeout_behavior(p4configv1::Table::NOTIFY_CONTROL);
        } else {
            table->set_idle_timeout_behavior(p4configv1::Table::NO_TIMEOUT);
        }
    }

    void addExternFunction(const P4RuntimeSymbolTableIface& symbols,
                           p4configv1::P4Info* p4info,
                           const P4::ExternFunction* externFunction) override {
        auto p4RtTypeInfo = p4info->mutable_type_info();
        auto digest = getDigestCall(externFunction, refMap, typeMap, p4RtTypeInfo);
        if (digest) addDigest(symbols, p4info, *digest);
    }

    /// @return serialization information for the digest() call represented by
    /// @call, or boost::none if @call is not a digest() call or is invalid.
    static boost::optional<Digest>
    getDigestCall(const P4::ExternFunction* function,
                  ReferenceMap* refMap,
                  P4::TypeMap* typeMap,
                  p4configv1::P4TypeInfo* p4RtTypeInfo) {
        if (function->method->name != P4V1::V1Model::instance.digest_receiver.name)
            return boost::none;

        auto call = function->expr;
        BUG_CHECK(call->typeArguments->size() == 1,
                  "%1%: Expected one type argument", call);
        BUG_CHECK(call->arguments->size() == 2, "%1%: Expected 2 arguments", call);

        // An invocation of digest() looks like this:
        //   digest<T>(receiver, { fields });
        // The name that shows up in the control plane API is the type name T. If T
        // doesn't have a name (e.g. tuple), we auto-generate one; ideally we would
        // be able to annotate the digest method call with a @name annotation in the
        // P4 but annotations are not supported on expressions.
        cstring controlPlaneName;
        auto* typeArg = call->typeArguments->at(0);
        if (typeArg->is<IR::Type_StructLike>()) {
            auto structType = typeArg->to<IR::Type_StructLike>();
            controlPlaneName = structType->controlPlaneName();
        } else if (auto* typeName = typeArg->to<IR::Type_Name>()) {
            auto* referencedType = refMap->getDeclaration(typeName->path, true);
            CHECK_NULL(referencedType);
            controlPlaneName = referencedType->controlPlaneName();
        } else {
            static std::unordered_map<const IR::MethodCallExpression*, cstring> autoNames;
            auto it = autoNames.find(call);
            if (it == autoNames.end()) {
              controlPlaneName = "digest_" + cstring::to_cstring(autoNames.size());
              ::warning(ErrorType::WARN_MISMATCH,
                        "Cannot find a good name for %1% method call, using "
                        "auto-generated name '%2%'", call, controlPlaneName);
              autoNames.emplace(call, controlPlaneName);
            } else {
              controlPlaneName = it->second;
            }
        }

        // Convert the generic type for the digest method call to a P4DataTypeSpec
        auto* typeSpec = TypeSpecConverter::convert(refMap, typeMap, typeArg, p4RtTypeInfo);
        BUG_CHECK(typeSpec != nullptr, "P4 type %1% could not "
                  "be converted to P4Info P4DataTypeSpec");
        return Digest{controlPlaneName, typeSpec, nullptr};
    }

    /// @return true if @table's 'support_timeout' property exists and is true. This
    /// indicates that @table supports entry ageing.
    static bool getSupportsTimeout(const IR::P4Table* table) {
        auto timeout = table->properties->getProperty(P4V1::V1Model::instance
                                                      .tableAttributes
                                                      .supportTimeout.name);
        if (timeout == nullptr) return false;
        if (!timeout->value->is<IR::ExpressionValue>()) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "Unexpected value %1% for supports_timeout on table %2%",
                    timeout, table);
            return false;
        }

        auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::BoolLiteral>()) {
            ::error(ErrorType::ERR_UNEXPECTED,
                    "Unexpected non-boolean value %1% for supports_timeout "
                    "property on table %2%", timeout, table);
            return false;
        }

        return expr->to<IR::BoolLiteral>()->value;
    }
};

P4RuntimeArchHandlerIface*
V1ModelArchHandlerBuilder::operator()(
    ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerV1Model(refMap, typeMap, evaluatedProgram);
}

/// Implements  a common @ref P4RuntimeArchHandlerIface for the PSA and PNA architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize PSA and PNA specific symbols which are exposed to the control-plane.
template <Arch arch>
class P4RuntimeArchHandlerPSAPNA : public P4RuntimeArchHandlerCommon<arch> {
 public:
    P4RuntimeArchHandlerPSAPNA(ReferenceMap* refMap,
                            TypeMap* typeMap,
                            const IR::ToplevelBlock* evaluatedProgram)
        : P4RuntimeArchHandlerCommon<arch>(refMap, typeMap, evaluatedProgram) {
    }

    void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                               const IR::ExternBlock* externBlock) override {
        P4RuntimeArchHandlerCommon<arch>::collectExternInstance(symbols, externBlock);

        auto decl = externBlock->node->to<IR::IDeclaration>();
        if (decl == nullptr) return;
        if (externBlock->type->name == "Digest") {
            symbols->add(SymbolType::DIGEST(), decl);
        }
    }

    void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                            p4configv1::P4Info* p4info,
                            p4configv1::Table* table,
                            const IR::TableBlock* tableBlock) override {
        P4RuntimeArchHandlerCommon<arch>::addTableProperties(
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
        P4RuntimeArchHandlerCommon<arch>::addExternInstance(
            symbols, p4info, externBlock);

        auto decl = externBlock->node->to<IR::Declaration_Instance>();
        if (decl == nullptr) return;
        auto p4RtTypeInfo = p4info->mutable_type_info();
        if (externBlock->type->name == "Digest") {
            auto digest = getDigest(decl, p4RtTypeInfo);
            if (digest) this->addDigest(symbols, p4info, *digest);
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
        auto typeSpec = TypeSpecConverter::convert(this->refMap, this->typeMap,
                                                   typeArg, p4RtTypeInfo);
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

class P4RuntimeArchHandlerPSA final : public P4RuntimeArchHandlerPSAPNA<Arch::PSA> {
 public:
    P4RuntimeArchHandlerPSA(ReferenceMap* refMap,
                             TypeMap* typeMap,
                             const IR::ToplevelBlock* evaluatedProgram)
            : P4RuntimeArchHandlerPSAPNA(refMap, typeMap, evaluatedProgram) {
     }
};

P4RuntimeArchHandlerIface*
PSAArchHandlerBuilder::operator()(
    ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerPSA(refMap, typeMap, evaluatedProgram);
}

class P4RuntimeArchHandlerPNA final : public P4RuntimeArchHandlerPSAPNA<Arch::PNA> {
 public:
    P4RuntimeArchHandlerPNA(ReferenceMap* refMap,
                             TypeMap* typeMap,
                             const IR::ToplevelBlock* evaluatedProgram)
            : P4RuntimeArchHandlerPSAPNA(refMap, typeMap, evaluatedProgram) {
     }
};

P4RuntimeArchHandlerIface*
PNAArchHandlerBuilder::operator()(
        ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerPNA(refMap, typeMap, evaluatedProgram);
}

/// Implements @ref P4RuntimeArchHandlerIface for the UBPF architecture.
/// We re-use PSA to handle externs.
/// Rationale: The only configurable extern object in ubpf_model.p4 is Register.
/// The Register is defined exactly the same as for PSA. Therefore, we can re-use PSA.
class P4RuntimeArchHandlerUBPF final : public P4RuntimeArchHandlerCommon<Arch::PSA> {
 public:
    P4RuntimeArchHandlerUBPF(ReferenceMap* refMap,
                             TypeMap* typeMap,
                             const IR::ToplevelBlock* evaluatedProgram)
            : P4RuntimeArchHandlerCommon<Arch::PSA>(refMap, typeMap, evaluatedProgram) { }
};

P4RuntimeArchHandlerIface*
UBPFArchHandlerBuilder::operator()(
        ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerUBPF(refMap, typeMap, evaluatedProgram);
}

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4
