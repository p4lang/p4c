/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 * \defgroup SimplifyEmitArgs BFN::SimplifyEmitArgs
 * \ingroup midend
 * \brief Set of passes that simplify headers and emits.
 *
 * This pass manager performs the following simplification on headers
 * and emit() methods.
 *
 * 1. The following:
 *
 *        header h {
 *            struct s {
 *                bit<8> f0;
 *            }
 *        }
 *
 *    is converted to
 *
 *        header h {
 *            bit<8> _s_f0;
 *        }
 *
 * 2. The following:
 *
 *        emit(hdr)
 *
 *    is converted to
 *
 *        emit({hdr.f0, hdr.f1})
 *
 * 3. The following:
 *
 *        header h {
 *            @flexible
 *            struct s0 {
 *                bit<8> f0;
 *            }
 *            @flexible
 *            struct s1 {
 *                bit<8> f0;
 *            }
 *        }
 *
 *    is converted to
 *
 *        header h {
 *            bit<8> _s0_f0 @flexible;
 *            bit<8> _s1_f0 @flexible;
 *        }
 *
 */
#ifndef EXTENSIONS_BF_P4C_MIDEND_SIMPLIFY_ARGS_H_
#define EXTENSIONS_BF_P4C_MIDEND_SIMPLIFY_ARGS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/cloner.h"
#include "midend/flattenHeaders.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/midend/check_header_alignment.h"
#include "bf-p4c/midend/copy_header.h"

namespace BFN {

/**
 * @class InjectTmpVar
 * @ingroup SimplifyEmitArgs
 * @brief PassManager which controls injecting of temporary variables to be used instead
 *        of nested structures which need to be flattened by FlattenHeader pass.
 *
 * Injection of temporary variables is done by pass DoInject.
 * As this pass introduces new declarations of variables, it has to be followed by
 * P4::MoveDeclarations pass.
 * As new variables have structure types and assignments of structures are already
 * converted to assignments of the individual fields before this pass, we call
 * also BFN::CopyHeaders pass to ensure that new assignments of the structures are
 * converted to the assignments of the individual fields too.
 *
 * This pass handles following cases:
 *
 * 1. Extern method returns a structure and this structure is assigned to a nested
 *    structure:
 *
 *        IR::AssignmentStatement
 *          IR::Member (left)
 *          IR::MethodCallExpression (right)
 *
 *    Let's assume following definitions of structures and header:
 *
 *        struct sB_t {
 *          bit<8>  f1;
 *          bit<16> f2;
 *        }
 *
 *        header hdr_t {
 *          sB_t sB;
 *        }
 *
 *        struct sA_t {
 *          hdr_t hdr;
 *        }
 *
 *    and a variable:
 *
 *        sA_t sA;
 *
 *    and an extern:
 *
 *        extern e {
 *          sB_t m1();
 *          void m2(inout sB_t p1);
 *        }
 *
 *    Then following:
 *
 *        sA.hdr.sB = e.m1();
 *
 *    is transformed to this:
 *
 *        sB_t tmp;
 *        tmp = e.m1();
 *        sA.hdr.sB.f1 = tmp.f1;
 *        sA.hdr.sB.f2 = tmp.f2;
 *
 * 2. Nested structure is passed as an argument to an extern method in a method
 *    call statement:
 *
 *        IR::MethodCallStatement
 *          IR::MethodCallExpression
 *
 *    Let's assume the same definitions as in previous case.
 *
 *    Then following:
 *
 *        e.m2(sA.hdr.sB);
 *
 *    is transformed to this:
 *
 *        sB_t p1;
 *        p1.f1 = sA.hdr.sB.f1; // in and inout parameters
 *        p1.f2 = sA.hdr.sB.f2;
 *        e.m2(p1);
 *        sA.hdr.sB.f1 = p1.f1; // out and inout parameters
 *        sA.hdr.sB.f2 = p2.f2;
 */
struct InjectTmpVar : public PassManager {
    /**
     * @brief Injects a temporary variable.
     * @post P4::MoveDeclarations and BFN::CopyHeaders passes need to be called after this one,
     *       to ensure that newly created declarations of variables are moved to the parent block
     *       and the assignments of the structures are replaced by assignments of the individual
     *       fields.
     */
    class DoInject : public Transform {
        P4::ReferenceMap *refMap;
        P4::TypeMap *typeMap;

        bool isNestedStruct(const IR::Expression *expr);

     public:
        explicit DoInject(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) :
                refMap(refMap), typeMap(typeMap) {}
        const IR::Node *postorder(IR::AssignmentStatement *as) override;
        const IR::Node *postorder(IR::MethodCallStatement *mcs) override;
    };

    InjectTmpVar(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        auto typeChecking = new BFN::TypeChecking(refMap, typeMap, true);
        passes.push_back(typeChecking);
        passes.push_back(new DoInject(refMap, typeMap));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(typeChecking);
        passes.push_back(new P4::MoveDeclarations());
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new BFN::CopyHeaders(refMap, typeMap, typeChecking));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(typeChecking);
    }
};

/**
 * \ingroup SimplifyEmitArgs
 * \brief Pass that flattened nested struct within a struct.
 *
 * This pass flattened nested struct within a struct, as well as
 * nested struct within a header.
 *
 * The corresponding pass in p4lang/p4c FlattenInterfaceStruct and
 * FlattenHeader create field name with a leading '_' and a trailing
 * number which is not compatible with how we name field in brig.
 * In addition, the FlattenInterfaceStruct pass does not seem to
 * work correctly, that is, typeChecking seems to fail after
 * the FlattenInterfaceStruct pass.
 *
 * So I decided to not use those two passes for now.
 *
 * @pre If a field which is a nested structure is passed as an argument to a method
 *      or a return value of a method is assigned to such field, the use of such field
 *      which is a structure can not be simply replaced by new fields created from the fields
 *      of that structure and this pass does not handle such cases. These situations can
 *      be eliminated by injecting temporary variables using pass InjectTmpVar before this pass.
 */
class FlattenHeader : public Modifier {
    P4::CloneExpressions cloner;
    const P4::TypeMap* typeMap;
    IR::Type_Header* flattenedHeader = nullptr;
    std::vector<cstring> nameSegments{};
    std::vector<const IR::Annotations*> allAnnotations{};
    std::vector<Util::SourceInfo> srcInfos{};
    cstring makeName(std::string_view sep) const;
    void flattenType(const IR::Type* type);
    const IR::Annotations* mergeAnnotations() const;

    const IR::Member* flattenedMember;
    std::vector<cstring> memberSegments{};
    std::map<cstring, cstring> fieldNameMap;
    std::map<cstring, std::tuple<const IR::Expression*, cstring>> replacementMap;
    cstring makeMember(std::string_view sep) const;
    void flattenMember(const IR::Member* member);
    const IR::Member* doFlattenMember(const IR::Member* member);

    std::vector<cstring> pathSegments{};
    cstring makePath(std::string_view sep) const;
    void flattenStructInitializer(const IR::StructExpression* e,
            IR::IndexedVector<IR::NamedExpression>* c);
    IR::StructExpression* doFlattenStructInitializer(const IR::StructExpression* e);
    IR::ListExpression* flatten_list(const IR::ListExpression* args);
    void explode(const IR::Expression*, IR::Vector<IR::Expression>*);
    int memberDepth(const IR::Member* m);
    const IR::Member *getTailMembers(const IR::Member* m, int depth);
    const IR::PathExpression *replaceSrcInfo(const IR::PathExpression *tgt,
                                             const IR::PathExpression *src);
    const IR::Member *replaceSrcInfo(const IR::Member *tgt, const IR::Member *src);
    const IR::Expression *balancedReplaceSrcInfo(const IR::Expression *tgt, const IR::Member *src);

    std::function<bool(const Context*, const IR::Type_StructLike*)> policy;

 public:
    explicit FlattenHeader(P4::TypeMap* typeMap,
            std::function<bool(const Context*, const IR::Type_StructLike*)> policy =
            [](const Context*, const IR::Type_StructLike*) -> bool { return false; }) :
        typeMap(typeMap), policy(policy) {}
    bool preorder(IR::Type_Header* header) override;
    bool preorder(IR::Member* member) override;
    void postorder(IR::Member* member) override;
    bool preorder(IR::MethodCallExpression* mc) override;
};

/**
 * \ingroup SimplifyEmitArgs
 *
 * Assume header type are flattend, no nested struct.
 */
class EliminateHeaders : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    std::function<bool(const Context*, const IR::Type_StructLike*)> policy;

 public:
    EliminateHeaders(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
            std::function<bool(const Context*, const IR::Type_StructLike*)> policy) :
        refMap(refMap), typeMap(typeMap), policy(policy) { setName("EliminateHeaders"); }
    std::map<cstring, IR::IndexedVector<IR::NamedExpression>> rewriteTupleType;
    std::map<const IR::MethodCallExpression* , const IR::Type*> rewriteOtherType;
    const IR::Node* preorder(IR::Argument* arg) override;
    void elimConcat(IR::IndexedVector<IR::NamedExpression>& output, const IR::Concat* expr);
};

/**
 * \ingroup SimplifyEmitArgs
 */
class RewriteTypeArguments : public Transform {
     const EliminateHeaders* eeh;
 public:
    explicit RewriteTypeArguments(const EliminateHeaders* eeh) : eeh(eeh) {}
    const IR::Node* preorder(IR::Type_Struct* type_struct) override;
    const IR::Node* preorder(IR::MethodCallExpression* mc) override;
};

/**
 * \ingroup SimplifyEmitArgs
 * \brief Top level PassManager that governs simplification of headers and emits.
 *
 * TODO: We can probably simplify this pass manager by combining
 * the following four passes into fewer passes.
 */
class SimplifyEmitArgs : public PassManager {
 public:
    SimplifyEmitArgs(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            std::function<bool(const Context *, const IR::Type_StructLike*)> policy =
            [](const Context*, const IR::Type_StructLike*) -> bool { return false; } ) {
        auto eliminateHeaders = new EliminateHeaders(refMap, typeMap, policy);
        auto rewriteTypeArguments = new RewriteTypeArguments(eliminateHeaders);
        passes.push_back(new InjectTmpVar(refMap, typeMap));
        passes.push_back(new FlattenHeader(typeMap));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new BFN::TypeChecking(refMap, typeMap, true));
        passes.push_back(eliminateHeaders);
        passes.push_back(rewriteTypeArguments);
        // After eliminateHeaders we need to do TypeInference that might
        // change new ListExpressions to StructExpressions
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new BFN::TypeChecking(refMap, typeMap, true));
        passes.push_back(new PadFlexibleField(refMap, typeMap)),
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new BFN::TypeChecking(refMap, typeMap, true));
    }
};

}  // namespace BFN

#endif /* EXTENSIONS_BF_P4C_MIDEND_SIMPLIFY_ARGS_H_ */
