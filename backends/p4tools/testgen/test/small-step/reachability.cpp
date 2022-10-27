#include "backends/p4tools/common/compiler/reachability.h"

#include <stdlib.h>

#include <fstream>

#include "backends/p4test/version.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/log.h"
#include "test/gtest/env.h"

namespace Test {

class P4ReachabilityOptions : public CompilerOptions {
 public:
    virtual ~P4ReachabilityOptions() = default;
    P4ReachabilityOptions() = default;
    P4ReachabilityOptions(const P4ReachabilityOptions&) = default;
    P4ReachabilityOptions(P4ReachabilityOptions&&) = delete;
    P4ReachabilityOptions& operator=(const P4ReachabilityOptions&) = default;
    P4ReachabilityOptions& operator=(P4ReachabilityOptions&&) = delete;
};

using P4ReachabilityContext = P4CContextWithOptions<P4ReachabilityOptions>;

class P4CReachability : public ::testing::Test {};

template <class T>
std::vector<const IR::IDeclaration*>* getNodeByType(const IR::P4Program* program) {
    std::function<bool(const IR::IDeclaration*)> filter = [](const IR::IDeclaration* d) {
        CHECK_NULL(d);
        return d->is<T>();
    };
    return program->getDeclarations()->where(filter)->toVector();
}

/// Loads example from a file
using ReturnedInfo = std::tuple<const IR::P4Program*, const P4Tools::NodesCallGraph*,
                                const P4Tools::ReachabilityHashType>;

ReturnedInfo loadExampleForReachability(const char* curFile) {
    AutoCompileContext autoP4TestContext(new P4ReachabilityContext());
    auto& options = P4ReachabilityContext::get().options();
    const char* argv = "./p4testgen";
    options.process(1, (char* const*)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    std::string includeDir = std::string(buildPath) + std::string("p4include");
    auto* originalEnv = getenv("P4C_16_INCLUDE_PATH");
    setenv("P4C_16_INCLUDE_PATH", includeDir.c_str(), 1);
    options.compilerVersion = P4TEST_VERSION_STRING;
    const IR::P4Program* program = nullptr;
    options.file = sourcePath;
    options.file += "testdata/p4_16_samples/";
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
    auto* currentDCG = new P4Tools::NodesCallGraph("NodesCallGraph");
    P4Tools::P4ProgramDCGCreator dcgCreator(&refMap, &typeMap, currentDCG);
    program->apply(dcgCreator);
    return std::make_tuple(program, currentDCG, currentDCG->getHash());
}

template <class T>
const IR::Node* getSpecificNode(std::vector<const IR::IDeclaration*>* v, cstring name) {
    CHECK_NULL(v);
    for (const auto* i : *v) {
        auto element = i->to<T>();
        if (element->name.name == name) {
            return element;
        }
    }
    return nullptr;
}

template <class T>
class NodeFinder : public Inspector {
 public:
    std::vector<const T*> v;
    bool preorder(const T* t) override {
        v.push_back(t);
        return false;
    }
};

TEST_F(P4CReachability, testParserStatesAndAnnotations) {
    auto result = loadExampleForReachability("action_profile-bmv2.p4");
    const auto* program = std::get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    auto* parserVector = getNodeByType<IR::P4Parser>(program);
    ASSERT_TRUE(parserVector);
    const auto* parser = getSpecificNode<IR::P4Parser>(parserVector, "ParserI");
    ASSERT_TRUE(parser);
    // Parser ParserI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, parser));
    auto* controlsVector = getNodeByType<IR::P4Control>(program);
    const auto* ingress = getSpecificNode<IR::P4Control>(controlsVector, "IngressI");
    ASSERT_TRUE(ingress);
    const auto* engress = getSpecificNode<IR::P4Control>(controlsVector, "EgressI");
    ASSERT_TRUE(engress);
    // IngressI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, ingress));
    // EgressI is reachable.
    ASSERT_TRUE(dcg->isReachable(program, engress));
    // EgressI is reachable from IngressI.
    ASSERT_TRUE(dcg->isReachable(ingress, engress));
    // IgressI is not reachable from EngressI.
    ASSERT_TRUE(!dcg->isReachable(engress, ingress));
    const auto* indirect =
        ingress->to<IR::P4Control>()->getDeclByName("indirect_0")->to<IR::P4Table>();
    ASSERT_TRUE(indirect);
    const auto* indirectWs =
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
    const auto* ap = findAnnotations.v[1];
    findAnnotations.v.clear();
    indirectWs->apply(findAnnotations);
    const auto* apWs = findAnnotations.v[2];
    // ap is reachable from igress.
    ASSERT_TRUE(dcg->isReachable(ingress, ap));
    // ap_ws is reachable from indirect.
    ASSERT_TRUE(dcg->isReachable(indirect, apWs));
    // ap is not reachable from engress.
    ASSERT_TRUE(!dcg->isReachable(engress, ap));
}

TEST_F(P4CReachability, testLoops) {
    auto result = loadExampleForReachability("stack_complex-bmv2.p4");
    const auto* program = std::get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
}

const IR::Node* getFromHash(const P4Tools::ReachabilityHashType& hash, const char* name) {
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
        "../../backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_hit.p4");
    const auto* program = get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto* ingress = getFromHash(hash, "ingress");
    ASSERT_TRUE(ingress);
    const auto* egress = getFromHash(hash, "egress");
    ASSERT_TRUE(egress);
    const auto* hitTable = getFromHash(hash, "ingress.hit_table");
    ASSERT_TRUE(hitTable);
    const auto* MyAction3 = getFromHash(hash, "ingress.MyAction3");
    ASSERT_TRUE(MyAction3);
    const auto* MyAction7 = getFromHash(hash, "ingress.MyAction7");
    ASSERT_TRUE(MyAction7);
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
    // MyAction7 is reachable from hit_table
    ASSERT_TRUE(dcg->isReachable(hitTable, MyAction7));
    // MyAction3 is reachable from hit_table
    ASSERT_TRUE(dcg->isReachable(hitTable, MyAction3));
    // MyAction3 is reachable from MyAction7
    ASSERT_TRUE(dcg->isReachable(MyAction7, MyAction3));
    // MyAction7 isn't reachable from MyAction3
    ASSERT_TRUE(!dcg->isReachable(MyAction3, MyAction7));
    // egress is reachable from MyAction3
    ASSERT_TRUE(dcg->isReachable(MyAction3, egress));
    // MyAction7 isn't reachable from egress
    ASSERT_TRUE(!dcg->isReachable(egress, MyAction7));
}

TEST_F(P4CReachability, testSwitchStatement) {
    auto result = loadExampleForReachability("basic_routing-bmv2.p4");
    const auto* program = get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto* ipv4Fib = getFromHash(hash, "ingress.ipv4_fib");
    ASSERT_TRUE(ipv4Fib);
    const auto* ipv4FibLpm = getFromHash(hash, "ingress.ipv4_fib_lpm");
    ASSERT_TRUE(ipv4FibLpm);
    const auto* nexthop = getFromHash(hash, "ingress.nexthop");
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
        "../../backends/p4tools/testgen/targets/bmv2/test/p4-programs/bmv2_if.p4");
    const auto* program = get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto* ingress = getFromHash(hash, "ingress");
    ASSERT_TRUE(ingress);
    const auto* myAction1 = getFromHash(hash, "ingress.MyAction1");
    ASSERT_TRUE(myAction1);
    const auto* myAction2 = getFromHash(hash, "ingress.MyAction2");
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
    auto result = loadExampleForReachability("value-sets.p4");
    const auto* program = get<0>(result);
    ASSERT_TRUE(program);
    const auto* dcg = std::get<1>(result);
    ASSERT_TRUE(dcg);
    auto hash = std::get<2>(result);
    const auto* start = getFromHash(hash, "start");
    ASSERT_TRUE(start);
    const auto* ethtypeKinds = getFromHash(hash, "TopParser.ethtype_kinds");
    ASSERT_TRUE(ethtypeKinds);
    const auto* parseTrill = getFromHash(hash, "parse_trill");
    ASSERT_TRUE(parseTrill);
    const auto* parseIpv4 = getFromHash(hash, "parse_ipv4");
    ASSERT_TRUE(parseIpv4);
    const auto* dispatchValueSets = getFromHash(hash, "dispatch_value_sets");
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

}  // namespace Test
