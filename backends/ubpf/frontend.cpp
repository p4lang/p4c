#include "ubpfModel.h"
#include "frontend.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "ir/pass_manager.h"

// Passes
#include "frontends/p4/actionsInlining.h"
#include "frontends/p4/checkConstants.h"
#include "frontends/p4/checkNamedArgs.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/defaultArguments.h"
#include "frontends/p4/deprecated.h"
#include "frontends/p4/directCalls.h"
#include "frontends/p4/dontcareArgs.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/functionsInlining.h"
#include "frontends/p4/hierarchicalNames.h"
#include "frontends/p4/inlining.h"
#include "frontends/p4/localizeActions.h"
#include "frontends/p4/moveConstructors.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/parserControlFlow.h"
#include "frontends/p4/removeReturns.h"
#include "frontends/p4/resetHeaders.h"
#include "frontends/p4/setHeaders.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyDefUse.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/specialize.h"
#include "frontends/p4/specializeGenericFunctions.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/structInitializers.h"
#include "frontends/p4/switchAddDefault.h"
#include "frontends/p4/tableKeyNames.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/p4/uselessCasts.h"
#include "frontends/p4/validateMatchAnnotations.h"
#include "frontends/p4/validateParsedProgram.h"

namespace UBPF {

class getUBPFModelVersion : public Inspector {
    bool preorder(const IR::Declaration_Constant *dc) override {
        if (dc->name == "__ubpf_model_version") {
            auto val = dc->initializer->to<IR::Constant>();
            UBPFModel::instance.version = static_cast<unsigned>(val->value); }
        return false; }
    bool preorder(const IR::Declaration *) override { return false; }
};

const IR::P4Program *FrontEnd::run(const CompilerOptions &options, const IR::P4Program *program,
                                   bool skipSideEffectOrdering, std::ostream *outStream) {
    if (program == nullptr && options.listFrontendPasses == 0)
        return nullptr;

    bool isv1 = options.isv1();
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    refMap.setIsV1(isv1);

    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    std::initializer_list<Visitor *> frontendPasses = {
            new UBPF::getUBPFModelVersion,
            // Parse annotations
            new P4::ParseAnnotationBodies(&parseAnnotations, &typeMap),
            new P4::PrettyPrint(options),
            // Simple checks on parsed program
            new P4::ValidateParsedProgram(),
            // Synthesize some built-in constructs
            new P4::CreateBuiltins(),
            new P4::ResolveReferences(&refMap, true),  // check shadowing
            // First pass of constant folding, before types are known --
            // may be needed to compute types.
            new P4::ConstantFolding(&refMap, nullptr),
            // Desugars direct parser and control applications
            // into instantiations followed by application
            new P4::InstantiateDirectCalls(&refMap),
            new P4::ResolveReferences(&refMap),  // check shadowing
            new P4::Deprecated(&refMap),
            new P4::CheckNamedArgs(),
            // Type checking and type inference.  Also inserts
            // explicit casts where implicit casts exist.
            new P4::TypeInference(&refMap, &typeMap, false),  // insert casts
            new P4::ValidateMatchAnnotations(&typeMap),
            new P4::BindTypeVariables(&refMap, &typeMap),
            // add default argument values to parameters
            new P4::DefaultArguments(&refMap, &typeMap),
            new P4::ResolveReferences(&refMap),
            new P4::TypeInference(&refMap, &typeMap, false),  // more casts may be needed
            new P4::RemoveParserControlFlow(&refMap, &typeMap),
            new P4::StructInitializers(&refMap, &typeMap),
            new P4::SpecializeGenericFunctions(&refMap, &typeMap),
            new P4::TableKeyNames(&refMap, &typeMap),
            new PassRepeated({
                                     new P4::ConstantFolding(&refMap, &typeMap),
                                     new P4::StrengthReduction(&refMap, &typeMap),
                                     new P4::UselessCasts(&refMap, &typeMap)
                             }),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::SwitchAddDefault,
            new P4::FrontEndDump(),  // used for testing the program at this point
            new P4::RemoveAllUnusedDeclarations(&refMap, true),
            new P4::SimplifyParsers(&refMap),
            new P4::ResetHeaders(&refMap, &typeMap),
            new P4::UniqueNames(&refMap),  // Give each local declaration a unique internal name
            new P4::MoveDeclarations(),  // Move all local declarations to the beginning
            new P4::MoveInitializers(&refMap),
            new P4::SideEffectOrdering(&refMap, &typeMap, skipSideEffectOrdering),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::MoveDeclarations(),  // Move all local declarations to the beginning
            new P4::SimplifyDefUse(&refMap, &typeMap),
            new P4::UniqueParameters(&refMap, &typeMap),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::SpecializeAll(&refMap, &typeMap),
            new P4::RemoveParserControlFlow(&refMap, &typeMap),
            new P4::RemoveReturns(&refMap),
            new P4::RemoveDontcareArgs(&refMap, &typeMap),
            new P4::MoveConstructors(&refMap),
            new P4::RemoveAllUnusedDeclarations(&refMap),
            new P4::ClearTypeMap(&typeMap),
            evaluator,
            new P4::Inline(&refMap, &typeMap, evaluator),
            new P4::InlineActions(&refMap, &typeMap),
            new P4::InlineFunctions(&refMap, &typeMap),
            new P4::SetHeaders(&refMap, &typeMap),
            // Check for constants only after inlining
            new P4::CheckConstants(&refMap, &typeMap),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::RemoveParserControlFlow(&refMap, &typeMap),
            new P4::UniqueNames(&refMap),
            new P4::LocalizeAllActions(&refMap),
            new P4::UniqueNames(&refMap),  // needed again after inlining
            new P4::UniqueParameters(&refMap, &typeMap),
            new P4::SimplifyControlFlow(&refMap, &typeMap),
            new P4::HierarchicalNames(),
            new P4::FrontEndLast(),
    };
    if (options.listFrontendPasses) {
        for (auto it : frontendPasses) {
            if (it != nullptr) {
                *outStream << it->name() << '\n';
            }
        }
        return nullptr;
    }

    PassManager passes(frontendPasses);

    if (options.excludeFrontendPasses) {
        passes.removePasses(options.passesToExcludeFrontend);
    }

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks, true);
    const IR::P4Program *result = program->apply(passes);
    return result;
}
}  // namespace UBPF
