#include "backends/p4tools/common/compiler/reachability.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>

#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeMap.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/compile_context.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "test/gtest/env.h"

namespace Test {

class P4ReachabilityOptions : public CompilerOptions {
 public:
    virtual ~P4ReachabilityOptions() = default;
    P4ReachabilityOptions() = default;
    P4ReachabilityOptions(const P4ReachabilityOptions &) = default;
    P4ReachabilityOptions(P4ReachabilityOptions &&) = delete;
    P4ReachabilityOptions &operator=(const P4ReachabilityOptions &) = default;
    P4ReachabilityOptions &operator=(P4ReachabilityOptions &&) = delete;
};

using P4ReachabilityContext = P4CContextWithOptions<P4ReachabilityOptions>;

class P4CReachability : public ::testing::Test {};

/// Loads example from a file
using ReturnedInfo = std::tuple<const IR::P4Program *, const P4Tools::NodesCallGraph *,
                                const P4Tools::ReachabilityHashType>;

ReturnedInfo loadExampleForReachability(const char *curFile) {
    AutoCompileContext autoP4TestContext(new P4ReachabilityContext());
    auto &options = P4ReachabilityContext::get().options();
    const char *argv = "./p4testgen";
    options.process(1, const_cast<char *const *>(&argv));
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto *originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    const IR::P4Program *program = nullptr;
    options.file = sourcePath;
    options.file += curFile;
    program = P4::parseP4File(options);
    if (originalEnv == nullptr) {
        unsetenv("P4C_16_INCLUDE_PATH");
    } else {
        setenv("P4C_16_INCLUDE_PATH", originalEnv, 1);
    }
    P4::FrontEnd frontend;
    program = frontend.run(options, program);
    CHECK_NULL(program);
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    P4Tools::MidEnd midEnd(options);
    program = program->apply(midEnd);
    auto *currentDCG = new P4Tools::NodesCallGraph("NodesCallGraph");
    P4Tools::P4ProgramDCGCreator dcgCreator(currentDCG);
    program->apply(dcgCreator);
    return std::make_tuple(program, currentDCG, currentDCG->getHash());
}

template <class T>
auto getNodeByType(const IR::P4Program *program) {
    return program->getDeclarations()->where([](const IR::IDeclaration *d) {
        CHECK_NULL(d);
        return d->is<T>();
    });
}

template <class Node>
const Node *getSpecificNode(const IR::P4Program *program, cstring name) {
    auto filter = [name](const IR::IDeclaration *d) {
        CHECK_NULL(d);
        if (const auto *element = d->to<Node>()) return element->name.name == name;

        return false;
    };
    if (auto *node = program->getDeclarations()->where(filter)->singleOrDefault())
        return node->template to<Node>();
    return nullptr;
}

template <class T>
class NodeFinder : public Inspector {
 public:
    std::vector<const T *> v;
    bool preorder(const T *t) override {
        v.push_back(t);
        return false;
    }
};

TEST_F(P4CReachability, testParserStatesAndAnnotations) {
    auto result = loadExampleForReachability("testdata/p4_16_samples/action_profile-bmv2.p4");
    const auto *program = std::get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    const auto *parser = getSpecificNode<IR::P4Parser>(program, "ParserI");
    ASSERT_TRUE(parser);
    // Parser ParserI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, parser));
    const auto *ingress = getSpecificNode<IR::P4Control>(program, "IngressI");
    ASSERT_TRUE(ingress);
    const auto *engress = getSpecificNode<IR::P4Control>(program, "EgressI");
    ASSERT_TRUE(engress);
    // IngressI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, ingress));
    // EgressI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, engress));
    // EgressI is reachable from IngressI.
    ASSERT_TRUE(dcg->isReachable(ingress, engress));
    // IgressI is not reachable from EngressI.
    ASSERT_TRUE(!dcg->isReachable(engress, ingress));
    const auto *indirect =
        ingress->to<IR::P4Control>()->getDeclByName("indirect_0")->to<IR::P4Table>();
    ASSERT_TRUE(indirect);
    const auto *indirectWs =
        ingress->to<IR::P4Control>()->getDeclByName("indirect_ws_0")->to<IR::P4Table>();
    ASSERT_TRUE(indirectWs);
    // Inderect table is reachable from ingress
    ASSERT_TRUE(dcg->isReachable(ingress, indirect));
    // Inderect_ws is not reachable from egress
    ASSERT_TRUE(!dcg->isReachable(engress, indirectWs));
    // Indirect_ws table is reachable from indirect
    ASSERT_TRUE(dcg->isReachable(indirect, indirectWs));
    // Indirect table is not reachable from indirect_ws
    ASSERT_TRUE(!dcg->isReachable(indirectWs, indirect));
    // Working with annotations.
    NodeFinder<IR::Annotation> findAnnotations;
    indirect->apply(findAnnotations);
    const auto *ap = findAnnotations.v[1];
    findAnnotations.v.clear();
    indirectWs->apply(findAnnotations);
    const auto *apWs = findAnnotations.v[2];
    // ap is reachable from igress.
    ASSERT_TRUE(dcg->isReachable(ingress, ap));
    // ap_ws is reachable from indirect.
    ASSERT_TRUE(dcg->isReachable(indirect, apWs));
    // ap is not reachable from engress.
    ASSERT_TRUE(!dcg->isReachable(engress, ap));
}

TEST_F(P4CReachability, testLoops) {
    auto result = loadExampleForReachability("testdata/p4_16_samples/stack_complex-bmv2.p4");
    const auto *program = std::get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
}

const IR::Node *getFromHash(const P4Tools::ReachabilityHashType &hash, const char *name) {
    CHECK_NULL(name);
    auto s = hash.find(name);
    if (s == hash.end()) {
        std::string nm = name;
        nm += "_table";
        s = hash.find(nm);
    }
    if (s == hash.end() || s->second.begin() == s->second.end()) {
        return nullptr;
    }
    return *s->second.begin();
}

TEST_F(P4CReachability, testTableAndActions) {
    auto result = loadExampleForReachability(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_hit.p4");
    const auto *program = get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto *ingress = getFromHash(hash, "ingress");
    ASSERT_TRUE(ingress);
    const auto *egress = getFromHash(hash, "egress");
    ASSERT_TRUE(egress);
    const auto *hitTable = getFromHash(hash, "ingress.hit_table");
    ASSERT_TRUE(hitTable);
    const auto *myAction3 = getFromHash(hash, "ingress.MyAction3");
    ASSERT_TRUE(myAction3);
    const auto *myAction7 = getFromHash(hash, "ingress.MyAction7");
    ASSERT_TRUE(myAction7);
    // egress is reachable from ingress.
    ASSERT_TRUE(dcg->isReachable(ingress, egress));
    // igress isn't reachable from egress.
    ASSERT_TRUE(!dcg->isReachable(egress, ingress));
    // hit_table is reachable from ingress.
    ASSERT_TRUE(dcg->isReachable(ingress, hitTable));
    // ingress isn't reachable from hit_table.
    ASSERT_TRUE(!dcg->isReachable(hitTable, ingress));
    // egress is reachable from hit_table.
    ASSERT_TRUE(dcg->isReachable(hitTable, egress));
    // myAction7 is reachable from hit_table
    ASSERT_TRUE(dcg->isReachable(hitTable, myAction7));
    // myAction3 is reachable from hit_table
    ASSERT_TRUE(dcg->isReachable(hitTable, myAction3));
    // myAction3 is reachable from myAction7
    ASSERT_TRUE(dcg->isReachable(myAction7, myAction3));
    // myAction7 isn't reachable from myAction3
    ASSERT_TRUE(!dcg->isReachable(myAction3, myAction7));
    // egress is reachable from myAction3
    ASSERT_TRUE(dcg->isReachable(myAction3, egress));
    // myAction7 isn't reachable from egress
    ASSERT_TRUE(!dcg->isReachable(egress, myAction7));
}

TEST_F(P4CReachability, testSwitchStatement) {
    auto result = loadExampleForReachability("testdata/p4_16_samples/basic_routing-bmv2.p4");
    const auto *program = get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto *ipv4Fib = getFromHash(hash, "ingress.ipv4_fib");
    ASSERT_TRUE(ipv4Fib);
    const auto *ipv4FibLpm = getFromHash(hash, "ingress.ipv4_fib_lpm");
    ASSERT_TRUE(ipv4FibLpm);
    const auto *nexthop = getFromHash(hash, "ingress.nexthop");
    ASSERT_TRUE(nexthop);
    // ipv4_fib_lpm is reachable from ipv4_fib.
    ASSERT_TRUE(dcg->isReachable(ipv4Fib, ipv4FibLpm));
    // ipv4_fib isn't reachable from ipv4_fib_lpm.
    ASSERT_TRUE(!dcg->isReachable(ipv4FibLpm, ipv4Fib));
    // nexthop is reachable from ipv4_fib.
    ASSERT_TRUE(dcg->isReachable(ipv4Fib, nexthop));
    // nexthop is reachable from ipv4_fib_lpm.
    ASSERT_TRUE(dcg->isReachable(ipv4FibLpm, nexthop));
    // ipv4_fib isn't reachable from nexthop.
    ASSERT_TRUE(!dcg->isReachable(nexthop, ipv4Fib));
    // ipv4_fib_lpm isn't reachable from nexthop.
    ASSERT_TRUE(!dcg->isReachable(nexthop, ipv4FibLpm));
}

TEST_F(P4CReachability, testIfStatement) {
    // Example for IsStatement checking.
    auto result = loadExampleForReachability(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4");
    const auto *program = get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto *ingress = getFromHash(hash, "ingress");
    ASSERT_TRUE(ingress);
    const auto *myAction1 = getFromHash(hash, "ingress.MyAction1");
    ASSERT_TRUE(myAction1);
    const auto *myAction2 = getFromHash(hash, "ingress.MyAction2");
    ASSERT_TRUE(myAction2);
    // MyAction1 is reachable from ingress.
    ASSERT_TRUE(dcg->isReachable(ingress, myAction1));
    // MyAction2 is reachable from ingress.
    ASSERT_TRUE(dcg->isReachable(ingress, myAction2));
    // MyAction2 is not reachable from MyAction1.
    ASSERT_TRUE(!dcg->isReachable(myAction1, myAction2));
    // MyAction1 is not reachable from MyAction2.
    ASSERT_TRUE(!dcg->isReachable(myAction2, myAction1));
}

TEST_F(P4CReachability, testParserValueSet) {
    auto result = loadExampleForReachability("testdata/p4_16_samples/value-sets.p4");
    const auto *program = get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto *start = getFromHash(hash, "start");
    ASSERT_TRUE(start);
    const auto *ethtypeKinds = getFromHash(hash, "TopParser.ethtype_kinds");
    ASSERT_TRUE(ethtypeKinds);
    const auto *parseTrill = getFromHash(hash, "parse_trill");
    ASSERT_TRUE(parseTrill);
    const auto *parseIpv4 = getFromHash(hash, "parse_ipv4");
    ASSERT_TRUE(parseIpv4);
    const auto *dispatchValueSets = getFromHash(hash, "dispatch_value_sets");
    ASSERT_TRUE(dispatchValueSets);
    // ethtype_kinds is reachable from start.
    ASSERT_TRUE(dcg->isReachable(start, ethtypeKinds));
    // ethtype_kinds is reachable from dispatch_value_sets.
    ASSERT_TRUE(dcg->isReachable(dispatchValueSets, ethtypeKinds));
    // ethtype_kinds isn't reachable from parseIpv4.
    ASSERT_TRUE(!dcg->isReachable(parseIpv4, ethtypeKinds));
    // ethtype_kinds isn't reachable from parse_trill.
    ASSERT_TRUE(!dcg->isReachable(parseTrill, ethtypeKinds));
}

bool listEqu(std::list<const IR::Node *> &left, std::list<const IR::Node *> right) {
    if (left.size() != right.size()) {
        return false;
    }
    auto i = left.begin();
    auto j = right.begin();
    while (i != left.end() && j != right.end()) {
        if (*i != *j) {
            return false;
        }
        i++;
        j++;
    }
    return true;
}

TEST_F(P4CReachability, testReachabilityEngine) {
    auto result = loadExampleForReachability(
        "backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4");
    const auto *program = get<0>(result);
    ASSERT_TRUE(program);
    const auto *dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    const auto hash = std::get<2>(result);
    std::string strBehavior = "ingress.MyAction1 + ingress.MyAction2;";
    strBehavior += "ingress.table2";
    P4Tools::ReachabilityEngine engine(*dcg, strBehavior);
    auto *engineState = P4Tools::ReachabilityEngineState::getInitial();
    // Initialize engine.
    const auto *ingress = getFromHash(hash, "ingress");
    ASSERT_TRUE(ingress);
    const auto *myAction1 = getFromHash(hash, "ingress.MyAction1");
    ASSERT_TRUE(myAction1);
    const auto *myAction2 = getFromHash(hash, "ingress.MyAction2");
    ASSERT_TRUE(myAction2);
    const auto *table2 = getFromHash(hash, "ingress.table2");
    ASSERT_TRUE(table2);
    const auto *p = getFromHash(hash, "p");
    ASSERT_TRUE(p);
    const auto *deparser = getFromHash(hash, "deparser");
    ASSERT_TRUE(deparser);
    // Move to p (engine state the same).
    // First it moves state to first user states.
    ASSERT_TRUE(engine.next(engineState, p).first);
    auto currentList = engineState->getState();
    // Move to ingress (engine state the same).
    ASSERT_TRUE(engine.next(engineState, ingress).first);
    ASSERT_TRUE(listEqu(currentList, engineState->getState()));
    // Move to MyAction1 (engine state should be changed).
    ASSERT_TRUE(engine.next(engineState, myAction1).first);
    ASSERT_TRUE(!listEqu(currentList, engineState->getState()));
    currentList = engineState->getState();
    // Can't move to MyAction2 after.
    ASSERT_TRUE(!engine.next(engineState, myAction2).first);
    ASSERT_TRUE(listEqu(currentList, engineState->getState()));
    // Move to table2 (engine sate should be changed).
    ASSERT_TRUE(engine.next(engineState, table2).first);
    ASSERT_TRUE(!listEqu(currentList, engineState->getState()));
    currentList = engineState->getState();
    // Any reachable next vertex should be accepted, move to mirroring_clone.
    ASSERT_TRUE(engine.next(engineState, deparser).first);
    ASSERT_TRUE(listEqu(currentList, engineState->getState()));
}

void callTestgen(const char *inputFile, const char *behavior, const char *path, int maxTests) {
    std::ostringstream mkDir;
    std::string prefix;
    std::string fullPath = sourcePath;
    fullPath += inputFile;
    mkDir << "mkdir -p " << buildPath << path << " && rm -f " << buildPath << path << "/*.stf";
    if (system(mkDir.str().c_str()) != 0) {
        BUG("Can't create folder - %1%", mkDir.str());
    }
    std::ostringstream cmdTestgen;
    cmdTestgen << buildPath << "p4testgen ";
    cmdTestgen << "-I \"" << buildPath << "p4include\" --target bmv2  --std p4-16 ";
    cmdTestgen << "--test-backend STF --arch v1model --seed 1000 --max-tests " << maxTests << "  ";
    cmdTestgen << "--pattern \"" << behavior << "\" ";
    cmdTestgen << "--out-dir \"" << buildPath << path << "\" \"" << sourcePath << prefix;
    cmdTestgen << inputFile << "\"";
    if (system(cmdTestgen.str().c_str()) != 0) {
        BUG("p4testgen failed to run - %1%", cmdTestgen.str());
    }
}

bool checkResultingSTF(std::list<std::list<std::string>> identifiersList, std::string path) {
    std::ostringstream resDir;
    resDir << buildPath << path;
    for (const auto &f : std::filesystem::directory_iterator(resDir.str())) {
        std::string fpath = f.path().c_str();
        if (fpath.rfind(".stf") == std::string::npos) {
            // Not a STF file.
            continue;
        }
        // Each STF file should contain all identifiers from identifiersList.
        auto lIds = identifiersList;
        std::ifstream ifile(f.path());
        while (!ifile.eof() && !lIds.empty()) {
            std::string line;
            std::getline(ifile, line);
            for (auto &lId : lIds) {
                if (lId.empty()) {
                    continue;
                }
                if (line.find(*lId.begin()) != std::string::npos) {
                    lId.pop_front();
                }
            }
        }
        ifile.close();
        bool hasEmptyList = false;
        for (auto &l : lIds) {
            if (l.empty()) {
                hasEmptyList = true;
                break;
            }
        }
        if (!hasEmptyList) {
            return false;
        }
    }
    return true;
}

TEST_F(P4CReachability, testReachabilityEngineActions) {
    callTestgen("backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_miss.p4",
                "ingress.MyAction2", "tmp", 10);
    std::list<std::list<std::string>> ids = {{"MyAction2"}};
    ASSERT_TRUE(checkResultingSTF(ids, "tmp"));
    ids = {{"MyAction1"}};
    ASSERT_TRUE(!checkResultingSTF(ids, "tmp"));
}

TEST_F(P4CReachability, testReachabilityEngineTables) {
    callTestgen("backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_action_run.p4",
                "ingress.MyAction2", "tmp", 10);
    std::list<std::list<std::string>> ids = {{" MyAction2"}};
    ASSERT_TRUE(checkResultingSTF(ids, "tmp"));
}

TEST_F(P4CReachability, testReachabilityEngineTable2) {
    callTestgen("backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4",
                "ingress.MyAction1;ingress.table2", "tmp", 10);
    std::list<std::list<std::string>> ids = {{"ingress.table2"}};
    ASSERT_TRUE(checkResultingSTF(ids, "tmp"));
}

TEST_F(P4CReachability, testReachabilityEngineNegTable2) {
    callTestgen("backends/p4tools/modules/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4",
                "!ingress.MyAction1;!ingress.table2", "tmp", 10);
    std::list<std::list<std::string>> ids = {{"table2"}};
    ASSERT_TRUE(!checkResultingSTF(ids, "tmp"));
}

}  // namespace Test
