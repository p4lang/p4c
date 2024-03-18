#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>

#include "frontends/common/parseInput.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "test/gtest/env.h"
#include "test/gtest/helpers.h"
#include "test/gtest/midend_pass.h"

using namespace P4;

namespace Test {

using P4TestContext = P4CContextWithOptions<CompilerOptions>;

class P4CParserUnroll : public P4CTest {};

// #define PARSER_UNROLL_TIME_CHECKING

#ifdef PARSER_UNROLL_TIME_CHECKING
#include <chrono>
#endif

const IR::P4Parser *getParser(const IR::P4Program *program) {
    // FIXME: This certainly should be improved, it should be possible to check
    // and cast at the same time
    return program->getDeclarations()
        ->where([](const IR::IDeclaration *d) { return d->is<IR::P4Parser>(); })
        ->single()
        ->to<IR::P4Parser>();
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
