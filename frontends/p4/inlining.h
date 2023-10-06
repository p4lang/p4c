/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef FRONTENDS_P4_INLINING_H_
#define FRONTENDS_P4_INLINING_H_

#include "commonInlining.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/ordered_map.h"

// These are various data structures needed by the parser/parser and control/control inliners.
// This only works correctly after local variable initializers have been removed,
// and after the SideEffectOrdering pass has been executed.

namespace P4 {

/// Describes information about a caller-callee pair
struct CallInfo : public IHasDbPrint {
    const IR::IContainer *caller;
    const IR::IContainer *callee;
    const IR::Declaration_Instance *instantiation;  // callee instantiation
    // Each instantiation may be invoked multiple times.
    std::set<const IR::MethodCallStatement *> invocations;  // all invocations within the caller

    CallInfo(const IR::IContainer *caller, const IR::IContainer *callee,
             const IR::Declaration_Instance *instantiation)
        : caller(caller), callee(callee), instantiation(instantiation) {
        CHECK_NULL(caller);
        CHECK_NULL(callee);
        CHECK_NULL(instantiation);
    }
    void addInvocation(const IR::MethodCallStatement *statement) { invocations.emplace(statement); }
    void dbprint(std::ostream &out) const {
        out << "Inline " << callee << " into " << caller << " with " << invocations.size()
            << " invocations";
    }
};

class SymRenameMap {
    std::map<const IR::IDeclaration *, cstring> internalName;
    std::map<const IR::IDeclaration *, cstring> externalName;

 public:
    void setNewName(const IR::IDeclaration *decl, cstring name, cstring extName) {
        CHECK_NULL(decl);
        BUG_CHECK(!name.isNullOrEmpty() && !extName.isNullOrEmpty(), "Empty name");
        LOG3("setNewName " << dbp(decl) << " to " << name);
        if (internalName.find(decl) != internalName.end()) BUG("%1%: already renamed", decl);
        internalName.emplace(decl, name);
        externalName.emplace(decl, extName);
    }
    cstring getName(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(internalName.find(decl) != internalName.end(), "%1%: no new name", decl);
        auto result = ::get(internalName, decl);
        return result;
    }
    cstring getExtName(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(externalName.find(decl) != externalName.end(), "%1%: no external name", decl);
        auto result = ::get(externalName, decl);
        return result;
    }
    bool isRenamed(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        return internalName.find(decl) != internalName.end();
    }
};

struct PerInstanceSubstitutions {
    ParameterSubstitution paramSubst;
    TypeVariableSubstitution tvs;
    SymRenameMap renameMap;
    PerInstanceSubstitutions() = default;
    PerInstanceSubstitutions(const PerInstanceSubstitutions &other)
        : paramSubst(other.paramSubst), tvs(other.tvs), renameMap(other.renameMap) {}
    template <class T>
    const T *rename(ReferenceMap *refMap, const IR::Node *node);
};

/// Summarizes all inline operations to be performed.
struct InlineSummary : public IHasDbPrint {
    /// Various substitutions that must be applied for each instance
    struct PerCaller {  // information for each caller
        /**
         * Pair identifying all the invocations of the subparser which can use the same
         * inlined states because the path after returning from the subparser is identical
         * for all such invocations.
         *
         * Invocations are characterized by:
         * - Pointer to the apply method call statement (comparing apply method call statement
         *   ensures that the same instance is invoked and same arguments are passed to the call)
         * - Pointer to the transition statement expression which has to be the name of
         *   the state (select expression is not allowed)
         *
         * Additional conditions which need to be met:
         * - there is no other statement between the invocation of the subparser and
         *   the transition statement
         * - transition statement has to be the name of the state (select expression is
         *   not allowed)
         *
         * @attention
         * Note that local variables declared in states calling subparser and passed to
         * the subparser as arguments need to be eliminated before Inline pass.
         * Currently this condition is met as passes UniqueNames and MoveDeclarations are
         * called before Inline pass in FrontEnd.
         * Otherwise (if this condition was not met) there could be different variables
         * with the same names passed as arguments to the apply method and additional
         * checks would have to be introduced to avoid optimization in such case.
         *
         * @see field invocationToState
         */
        typedef std::pair<const IR::MethodCallStatement *, const IR::PathExpression *>
            InlinedInvocationInfo;

        /**
         * Hash for InlinedInvocationInfo used as a key for unordered_map
         *
         * @see field invocationToState
         */
        struct key_hash {
            std::size_t operator()(const InlinedInvocationInfo &k) const {
                std::ostringstream oss;
                std::get<0>(k)->dbprint(oss);
                std::get<1>(k)->dbprint(oss);
                return std::hash<std::string>{}(oss.str());
            }
        };

        /**
         * Binary equality predicate for InlinedInvocationInfo used as a key for unordered_map
         *
         * @see field invocationToState
         */
        struct key_equal {
            bool operator()(const InlinedInvocationInfo &v0,
                            const InlinedInvocationInfo &v1) const {
                return std::get<0>(v0)->equiv(*std::get<0>(v1)) &&
                       std::get<1>(v0)->equiv(*std::get<1>(v1));
            }
        };

        /// For each instance (key) the container that is intantiated.
        std::map<const IR::Declaration_Instance *, const IR::IContainer *> declToCallee;
        /// For each instance (key) we must apply a bunch of substitutions
        std::map<const IR::Declaration_Instance *, PerInstanceSubstitutions *> substitutions;
        /// For each invocation (key) call the instance that is invoked.
        std::map<const IR::MethodCallStatement *, const IR::Declaration_Instance *> callToInstance;

        /**
         * For each distinct invocation of the subparser identified by InlinedInvocationInfo
         * we store the ID of the next caller parser state which is a new state replacing
         * the start state of the callee parser (subparser).
         * Transition to this state is used in case there is another subparser invocation
         * which has the equivalent InlinedInvocationInfo.
         *
         * Subparser invocations are considered to be equivalent when following conditions
         * are met (which means that the path in the parse graph is the same after returning
         * from subparser call):
         * - apply method call statements of the invocations are equivalent (which means that
         *   the same subparser instance is invoked and the same arguments are passed to
         *   the apply method)
         * - there is no statement between apply method call statement and transition statement
         * - transition statement is a name of the state (not a select expression)
         * - name of the state in transition statement is the same
         *
         *
         * Example of the optimization
         *
         * Parser and subparser before Inline pass:
         * @code{.p4}
         * parser Subparser(packet_in packet, inout data_t hdr) {
         *     state start {
         *         hdr.f = 8w42;
         *         transition accept;
         *     }
         * }
         * parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta,
         *                   inout standard_metadata_t standard_metadata) {
         *     @name("p") Subparser() p_0;
         *     state start {
         *         transition select(standard_metadata.ingress_port) {
         *             9w0: p0;
         *             default: p1;
         *         }
         *     }
         *     state p0 {
         *         p_0.apply(packet, hdr.h1);
         *         transition accept;
         *     }
         *     state p1 {
         *         p_0.apply(packet, hdr.h1);
         *         transition accept;
         *     }
         * }
         * @endcode
         *
         * Parser after Inline pass without optimization:
         * @code{.p4}
         * parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout
         * standard_metadata_t standard_metadata) { state start { transition
         * select(standard_metadata.ingress_port) { 9w0: p0; default: p1;
         *         }
         *     }
         *     state p0 {
         *         transition Subparser_start;
         *     }
         *     state Subparser_start {
         *         hdr.h1.f = 8w42;
         *         transition p0_0;
         *     }
         *     state p0_0 {
         *         transition accept;
         *     }
         *     state p1 {
         *         transition Subparser_start_0;
         *     }
         *     state Subparser_start_0 {
         *         hdr.h1.f = 8w42;
         *         transition p1_0;
         *     }
         *     state p1_0 {
         *         transition accept;
         *     }
         * }
         * @endcode
         *
         * Parser after Inline pass with optimization:
         * @code{.p4}
         * parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout
         * standard_metadata_t standard_metadata) { state start { transition
         * select(standard_metadata.ingress_port) { 9w0: p0; default: p1;
         *         }
         *     }
         *     state p0 {
         *         transition Subparser_start;
         *     }
         *     state Subparser_start {
         *         hdr.h1.f = 8w42;
         *         transition p0_0;
         *     }
         *     state p0_0 {
         *         transition accept;
         *     }
         *     state p1 {
         *         transition Subparser_start;
         *     }
         * }
         * @endcode
         *
         * @attention
         * This field is used only when the optimization of parser inlining is enabled,
         * otherwise it is not used.
         * The optimization is disabled by default.
         * The optimization can be enabled using command line option
         * --parser-inline-opt.
         */
        std::unordered_map<const InlinedInvocationInfo, const IR::ID, key_hash, key_equal>
            invocationToState;

        /// @returns nullptr if there isn't exactly one caller,
        /// otherwise the single caller of this instance.
        const IR::MethodCallStatement *uniqueCaller(
            const IR::Declaration_Instance *instance) const {
            const IR::MethodCallStatement *call = nullptr;
            for (auto m : callToInstance) {
                if (m.second == instance) {
                    if (call == nullptr)
                        call = m.first;
                    else
                        return nullptr;
                }
            }
            return call;
        }
    };
    std::map<const IR::IContainer *, PerCaller> callerToWork;

    void add(const CallInfo *cci) {
        callerToWork[cci->caller].declToCallee[cci->instantiation] = cci->callee;
        for (auto mcs : cci->invocations)
            callerToWork[cci->caller].callToInstance[mcs] = cci->instantiation;
    }
    void dbprint(std::ostream &out) const {
        out << "Inline " << callerToWork.size() << " call sites";
    }
};

// Inling information constructed here.
class InlineList {
    // We use an ordered map to make the iterator deterministic
    ordered_map<const IR::Declaration_Instance *, CallInfo *> inlineMap;
    std::vector<CallInfo *> toInline;  // sorted in order of inlining
    const bool allowMultipleCalls = true;

 public:
    void addInstantiation(const IR::IContainer *caller, const IR::IContainer *callee,
                          const IR::Declaration_Instance *instantiation) {
        CHECK_NULL(caller);
        CHECK_NULL(callee);
        CHECK_NULL(instantiation);
        LOG3("Inline instantiation " << dbp(instantiation));
        auto inst = new CallInfo(caller, callee, instantiation);
        inlineMap[instantiation] = inst;
    }

    size_t size() const { return inlineMap.size(); }

    void addInvocation(const IR::Declaration_Instance *instance,
                       const IR::MethodCallStatement *statement) {
        CHECK_NULL(instance);
        CHECK_NULL(statement);
        LOG3("Inline invocation " << dbp(instance) << " at " << dbp(statement));
        auto info = inlineMap[instance];
        BUG_CHECK(info, "Could not locate instance %1% invoked by %2%", instance, statement);
        info->addInvocation(statement);
    }

    void replace(const IR::IContainer *container, const IR::IContainer *replacement) {
        CHECK_NULL(container);
        CHECK_NULL(replacement);
        LOG3("Replacing " << dbp(container) << " with " << dbp(replacement));
        for (auto e : toInline) {
            if (e->callee == container) e->callee = replacement;
            if (e->caller == container) e->caller = replacement;
        }
    }

    void analyze();
    InlineSummary *next();
};

/// Must be run after an evaluator; uses the blocks to discover caller/callee relationships.
class DiscoverInlining : public Inspector {
    InlineList *inlineList;  // output: result is here
    ReferenceMap *refMap;    // input
    TypeMap *typeMap;        // input
    IHasBlock *evaluator;    // used to obtain the toplevel block
    IR::ToplevelBlock *toplevel;

 public:
    bool allowParsers = true;
    bool allowControls = true;

    DiscoverInlining(InlineList *inlineList, ReferenceMap *refMap, TypeMap *typeMap,
                     IHasBlock *evaluator)
        : inlineList(inlineList),
          refMap(refMap),
          typeMap(typeMap),
          evaluator(evaluator),
          toplevel(nullptr) {
        CHECK_NULL(inlineList);
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(evaluator);
        setName("DiscoverInlining");
        visitDagOnce = false;
    }
    Visitor::profile_t init_apply(const IR::Node *node) override {
        toplevel = evaluator->getToplevelBlock();
        CHECK_NULL(toplevel);
        return Inspector::init_apply(node);
    }
    void visit_all(const IR::Block *block);
    bool preorder(const IR::Block *block) override {
        visit_all(block);
        return false;
    }
    bool preorder(const IR::ControlBlock *block) override;
    bool preorder(const IR::ParserBlock *block) override;
    void postorder(const IR::MethodCallStatement *statement) override;
    // We don't care to visit the program, we just visit the blocks.
    bool preorder(const IR::P4Program *) override {
        visit_all(toplevel);
        return false;
    }
};

/// Performs actual inlining work
class GeneralInliner : public AbstractInliner<InlineList, InlineSummary> {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    InlineSummary::PerCaller *workToDo;
    bool optimizeParserInlining;

 public:
    explicit GeneralInliner(bool isv1, bool _optimizeParserInlining)
        : refMap(new ReferenceMap()),
          typeMap(new TypeMap()),
          workToDo(nullptr),
          optimizeParserInlining(_optimizeParserInlining) {
        setName("GeneralInliner");
        refMap->setIsV1(isv1);
        visitDagOnce = false;
    }
    // controlled visiting order
    const IR::Node *preorder(IR::MethodCallStatement *statement) override;
    /** Build the substitutions needed for args and locals of the thing being inlined.
     * P4Block here should be either P4Control or P4Parser.
     * P4BlockType should be either Type_Control or Type_Parser to match the P4Block.
     */
    template <class P4Block, class P4BlockType>
    void inline_subst(P4Block *caller, IR::IndexedVector<IR::Declaration> P4Block::*blockLocals,
                      const P4BlockType *P4Block::*blockType);
    const IR::Node *preorder(IR::P4Control *caller) override;
    const IR::Node *preorder(IR::P4Parser *caller) override;
    const IR::Node *preorder(IR::ParserState *state) override;
    Visitor::profile_t init_apply(const IR::Node *node) override;
};

/// Performs one round of inlining bottoms-up
class InlinePass : public PassManager {
    InlineList toInline;

 public:
    InlinePass(ReferenceMap *refMap, TypeMap *typeMap, EvaluatorPass *evaluator,
               bool optimizeParserInlining)
        : PassManager({new TypeChecking(refMap, typeMap),
                       new DiscoverInlining(&toInline, refMap, typeMap, evaluator),
                       new InlineDriver<InlineList, InlineSummary>(
                           &toInline, new GeneralInliner(refMap->isV1(), optimizeParserInlining)),
                       new RemoveAllUnusedDeclarations(refMap)}) {
        setName("InlinePass");
    }
};

/**
Performs inlining as many times as necessary.  Most frequently once
will be enough.  Multiple iterations are necessary only when instances are
passed as arguments using constructor arguments.
*/
class Inline : public PassRepeated {
    static std::set<cstring> noPropagateAnnotations;

 public:
    Inline(ReferenceMap *refMap, TypeMap *typeMap, EvaluatorPass *evaluator,
           bool optimizeParserInlining)
        : PassManager({new InlinePass(refMap, typeMap, evaluator, optimizeParserInlining),
                       // After inlining the output of the evaluator changes, so
                       // we have to run it again
                       evaluator}) {
        setName("Inline");
    }

    /// Do not propagate annotation \p name during inlining
    static void setAnnotationNoPropagate(cstring name) { noPropagateAnnotations.emplace(name); }

    /// Is annotation \p name excluded from inline propagation?
    static bool isAnnotationNoPropagate(cstring name) { return noPropagateAnnotations.count(name); }
};

}  // namespace P4

#endif /* FRONTENDS_P4_INLINING_H_ */
