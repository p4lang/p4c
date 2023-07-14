#include <cstdlib>
#include <fstream>

#include "frontends/common/constantFolding.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/unusedDeclarations.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "midend/actionSynthesis.h"
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/copyStructures.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateSwitch.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/flattenHeaders.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/nestedStructs.h"
#include "midend/noMatch.h"
#include "midend/parserUnroll.h"
#include "midend/predication.h"
#include "midend/removeAssertAssume.h"
#include "midend/removeExits.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/tableHit.h"
#include "test/gtest/env.h"
#include "test/gtest/helpers.h"

using namespace P4;

namespace Test {

class P4TestOptions : public CompilerOptions {
 public:
    P4TestOptions() {}
};

using P4TestContext = P4CContextWithOptions<P4TestOptions>;

class P4CParserUnroll : public P4CTest {};

class SkipControls : public P4::ActionSynthesisPolicy {
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const Visitor::Context *, const IR::P4Control *control) override {
        if (skip->find(control->name) != skip->end()) return false;
        return true;
    }
};

class MidEnd : public PassManager {
 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::ToplevelBlock *toplevel = nullptr;

    explicit MidEnd(CompilerOptions &options, std::ostream *outStream = nullptr) {
        bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
        refMap.setIsV1(isv1);
        auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
        setName("MidEnd");

        auto v1controls = new std::set<cstring>();

        addPasses(
            {options.ndebug ? new P4::RemoveAssertAssume(&refMap, &typeMap) : nullptr,
             new P4::RemoveMiss(&refMap, &typeMap),
             new P4::EliminateNewtype(&refMap, &typeMap),
             new P4::EliminateSerEnums(&refMap, &typeMap),
             new P4::SimplifyKey(
                 &refMap, &typeMap,
                 new P4::OrPolicy(new P4::IsValid(&refMap, &typeMap), new P4::IsLikeLeftValue())),
             new P4::RemoveExits(&refMap, &typeMap),
             new P4::ConstantFolding(&refMap, &typeMap),
             new P4::SimplifySelectCases(&refMap, &typeMap, false),  // non-constant keysets
             new P4::ExpandLookahead(&refMap, &typeMap),
             new P4::ExpandEmit(&refMap, &typeMap),
             new P4::HandleNoMatch(&refMap),
             new P4::SimplifyParsers(&refMap),
             new P4::StrengthReduction(&refMap, &typeMap),
             new P4::EliminateTuples(&refMap, &typeMap),
             new P4::SimplifyComparisons(&refMap, &typeMap),
             new P4::CopyStructures(&refMap, &typeMap),
             new P4::NestedStructs(&refMap, &typeMap),
             new P4::SimplifySelectList(&refMap, &typeMap),
             new P4::RemoveSelectBooleans(&refMap, &typeMap),
             new P4::FlattenHeaders(&refMap, &typeMap),
             new P4::FlattenInterfaceStructs(&refMap, &typeMap),
             new P4::ReplaceSelectRange(&refMap, &typeMap),
             new P4::Predication(&refMap),
             new P4::MoveDeclarations(),  // more may have been introduced
             new P4::ConstantFolding(&refMap, &typeMap),
             new P4::LocalCopyPropagation(&refMap, &typeMap),
             new P4::ConstantFolding(&refMap, &typeMap),
             new P4::StrengthReduction(&refMap, &typeMap),
             new P4::MoveDeclarations(),  // more may have been introduced
             new P4::SimplifyControlFlow(&refMap, &typeMap),
             new P4::CompileTimeOperations(),
             new P4::TableHit(&refMap, &typeMap),
             new P4::EliminateSwitch(&refMap, &typeMap),
             evaluator,
             [v1controls, evaluator](const IR::Node *root) -> const IR::Node * {
                 auto toplevel = evaluator->getToplevelBlock();
                 auto main = toplevel->getMain();
                 if (main == nullptr)
                     // nothing further to do
                     return nullptr;
                 // Special handling when compiling for v1model.p4
                 if (main->type->name == P4V1::V1Model::instance.sw.name) {
                     if (main->getConstructorParameters()->size() != 6) return root;
                     auto verify = main->getParameterValue(P4V1::V1Model::instance.sw.verify.name);
                     auto update = main->getParameterValue(P4V1::V1Model::instance.sw.compute.name);
                     auto deparser =
                         main->getParameterValue(P4V1::V1Model::instance.sw.deparser.name);
                     if (verify == nullptr || update == nullptr || deparser == nullptr ||
                         !verify->is<IR::ControlBlock>() || !update->is<IR::ControlBlock>() ||
                         !deparser->is<IR::ControlBlock>()) {
                         return root;
                     }
                     v1controls->emplace(verify->to<IR::ControlBlock>()->container->name);
                     v1controls->emplace(update->to<IR::ControlBlock>()->container->name);
                     v1controls->emplace(deparser->to<IR::ControlBlock>()->container->name);
                 }
                 return root;
             },
             new P4::SynthesizeActions(&refMap, &typeMap, new SkipControls(v1controls)),
             new P4::MoveActionsToTables(&refMap, &typeMap),
             options.loopsUnrolling ? new ParsersUnroll(true, &refMap, &typeMap) : nullptr,
             evaluator,
             [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
             new P4::MidEndLast()});
        if (options.listMidendPasses) {
            listPasses(*outStream, "\n");
            *outStream << std::endl;
            return;
        }
        if (options.excludeMidendPasses) {
            removePasses(options.passesToExcludeMidend);
        }
        toplevel = evaluator->getToplevelBlock();
    }
    IR::ToplevelBlock *process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel;
    }
};

// #define PARSER_UNROLL_TIME_CHECKING

#ifdef PARSER_UNROLL_TIME_CHECKING
#include <chrono>
#endif

const IR::P4Parser *getParser(const IR::P4Program *program) {
    std::function<bool(const IR::IDeclaration *)> filter = [](const IR::IDeclaration *d) {
        CHECK_NULL(d);
        return d->is<IR::P4Parser>();
    };
    const auto *newDeclVector = program->getDeclarations()->where(filter)->toVector();
    return (*newDeclVector)[0]->to<IR::P4Parser>();
}

/// Rewrites parser
std::pair<const IR::P4Parser *, const IR::P4Parser *> rewriteParser(const IR::P4Program *program,
                                                                    CompilerOptions &options) {
    P4::FrontEnd frontend;
    program = frontend.run(options, program);
    CHECK_NULL(program);
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

#ifdef PARSER_UNROLL_TIME_CHECKING
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    auto t1 = high_resolution_clock::now();
#endif
    MidEnd midEnd(options);
    const IR::P4Program *res = program;
    midEnd.process(res);
#ifdef PARSER_UNROLL_TIME_CHECKING
    auto t2 = high_resolution_clock::now();
    auto msInt = duration_cast<milliseconds>(t2 - t1);
    std::cout << msInt.count() << std::endl;
#endif
    return std::make_pair(getParser(program), getParser(res));
}

/// Loads example from a file
const IR::P4Program *load_model(const char *curFile, CompilerOptions &options) {
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    options.loopsUnrolling = true;
    options.file = sourcePath;
    options.file += "testdata/p4_16_samples/";
    options.file += curFile;
    auto program = P4::parseP4File(options);
    if (!originalEnv)
        unsetenv("P4C_16_INCLUDE_PATH");
    else
        setenv("P4C_16_INCLUDE_PATH", originalEnv, 1);
    return program;
}

std::pair<const IR::P4Parser *, const IR::P4Parser *> loadExample(
    const char *file,
    CompilerOptions::FrontendVersion langVersion = CompilerOptions::FrontendVersion::P4_16) {
    AutoCompileContext autoP4TestContext(new P4TestContext);
    auto &options = P4TestContext::get().options();
    const char *argv = "./gtestp4c";
    options.process(1, (char *const *)&argv);
    options.langVersion = langVersion;
    const IR::P4Program *program = load_model(file, options);
    if (!program) return std::make_pair(nullptr, nullptr);
    return rewriteParser(program, options);
}

TEST_F(P4CParserUnroll, test1) {
    auto parsers = loadExample("parser-unroll-test1.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    // 2 - exclude accept and reject states
    // adding MAX_HOPS and outOfBound states
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size() - 3 - 1);
}

TEST_F(P4CParserUnroll, test2) {
    auto parsers = loadExample("parser-unroll-test2.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size() - 3 - 1);
}

TEST_F(P4CParserUnroll, test3) {
    auto parsers = loadExample("parser-unroll-test3.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size() - 4);
}

TEST_F(P4CParserUnroll, switch_20160512) {
    auto parsers = loadExample("../p4_14_samples/switch_20160512/switch.p4",
                               CompilerOptions::FrontendVersion::P4_14);
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size() - 22 - 4 - 2);
}

TEST_F(P4CParserUnroll, header_stack_access_remover) {
    auto parsers = loadExample("parser-unroll-test4.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size() - 3);
}

TEST_F(P4CParserUnroll, noLoopsAndHeaderStacks) {
    auto parsers = loadExample("parser-unroll-test5.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size());
}

TEST_F(P4CParserUnroll, t1_Cond) {
    auto parsers = loadExample("parser-unroll-t1-cond.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size());
}

TEST_F(P4CParserUnroll, header_union) {
    auto parsers = loadExample("issue561-7-bmv2.p4");
    ASSERT_TRUE(parsers.first);
    ASSERT_TRUE(parsers.second);
    ASSERT_EQ(parsers.first->states.size(), parsers.second->states.size());
}

}  // namespace Test
