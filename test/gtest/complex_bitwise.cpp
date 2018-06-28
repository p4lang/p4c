#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "helpers.h"
#include "lib/log.h"
#include "lib/sourceCodeBuilder.h"

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "midend/simplifyBitwise.h"

using namespace P4;


namespace Test {

namespace {

boost::optional<FrontendTestCase>
createSimplifyBitwiseTestCase(const std::string &ingressSource) {
    std::string source = P4_SOURCE(P4Headers::V1MODEL, R"(
header H
{
   bit<32> f1;
   bit<32> f2;
   bit<32> f3;
}

struct Headers { H h; }
struct Metadata { }

parser parse(packet_in packet, out Headers headers, inout Metadata meta,
         inout standard_metadata_t sm) {
    state start {
        packet.extract(headers.h);
        transition accept;
    }
}

control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }
control ingress(inout Headers headers, inout Metadata meta,
                inout standard_metadata_t sm) {
    apply {
%INGRESS%
    }
}

control egress(inout Headers headers, inout Metadata meta,
                inout standard_metadata_t sm) { apply { } }

control computeChecksum(inout Headers headers, inout Metadata meta) { apply { } }

control deparse(packet_out packet, in Headers headers) {
    apply { packet.emit(headers.h); }
}

V1Switch(parse(), verifyChecksum(), ingress(), egress(),
    computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS%", ingressSource);
    return FrontendTestCase::create(source, CompilerOptions::FrontendVersion::P4_16);
}

class CountAssignmentStatements : public Inspector {
    int _as_total = 0;
    bool preorder(const IR::AssignmentStatement *) {
        _as_total++;
        return true;
    }

 public:
    int as_total() { return _as_total; }
};

}  // namespace

class SimplifyBitwiseTest : public P4CTest { };

TEST_F(SimplifyBitwiseTest, SimpleSplit) {
    auto test = createSimplifyBitwiseTestCase(P4_SOURCE(R"(
        headers.h.f1 = headers.h.f2 & 0xffff | headers.h.f1 & 0xffff0000;
    )"));

    ReferenceMap refMap;
    TypeMap typeMap;

    CountAssignmentStatements cas;
    Util::SourceCodeBuilder builder;
    ToP4 dump(builder, false);

    PassManager quick_midend = {
        new TypeChecking(&refMap, &typeMap, true),
        new SimplifyBitwise,
        &cas,
        &dump
    };

    test->program->apply(quick_midend);
    EXPECT_EQ(2, cas.as_total());

    std::string program_string = builder.toString();
    std::string value1 = "headers.h.f1[15:0] = headers.h.f2[15:0]";
    std::string value2 = "headers.h.f1[31:16] = headers.h.f1[31:16]";
    EXPECT_FALSE(program_string.find(value1) == std::string::npos);
    EXPECT_FALSE(program_string.find(value2) == std::string::npos);
}

TEST_F(SimplifyBitwiseTest, ManySplit) {
    auto test = createSimplifyBitwiseTestCase(P4_SOURCE(R"(
        headers.h.f1 = headers.h.f2 & 0x55555555 | headers.h.f1 & 0xaaaaaaaa;
    )"));

    ReferenceMap refMap;
    TypeMap typeMap;

    CountAssignmentStatements cas;
    Util::SourceCodeBuilder builder;
    ToP4 dump(builder, false);

    PassManager quick_midend = {
        new TypeChecking(&refMap, &typeMap, true),
        new SimplifyBitwise,
        &cas,
        &dump
    };

    test->program->apply(quick_midend);
    EXPECT_EQ(32, cas.as_total());
    std::string program_string = builder.toString();
    for (int i = 0; i < 32; i += 2) {
        std::string value1 = "headers.h.f1[" + std::to_string(i) + ":" + std::to_string(i)
                              + "] = headers.h.f2[" + std::to_string(i) + ":"
                              + std::to_string(i) + "]";
        std::string value2 = "headers.h.f1[" + std::to_string(i+1) + ":" + std::to_string(i+1)
                              + "] = headers.h.f1[" + std::to_string(i+1) + ":"
                              + std::to_string(i+1) + "]";
        EXPECT_FALSE(program_string.find(value1) == std::string::npos);
        EXPECT_FALSE(program_string.find(value2) == std::string::npos);
    }
}

TEST_F(SimplifyBitwiseTest, SplitWithZero) {
    auto test = createSimplifyBitwiseTestCase(P4_SOURCE(R"(
        headers.h.f1 = headers.h.f2 & 0xff | headers.h.f3 & 0xff000000;
    )"));

    ReferenceMap refMap;
    TypeMap typeMap;
    CountAssignmentStatements cas;

    Util::SourceCodeBuilder builder;
    ToP4 dump(builder, false);
    PassManager quick_midend = {
        new TypeChecking(&refMap, &typeMap, true),
        new SimplifyBitwise,
        &cas,
        &dump
    };

    test->program->apply(quick_midend);
    EXPECT_EQ(3, cas.as_total());
    std::string program_string = builder.toString();

    std::string value1 = "headers.h.f1[7:0] = headers.h.f2[7:0]";
    std::string value2 = "headers.h.f1[31:24] = headers.h.f3[31:24]";
    std::string value3 = "headers.h.f1[23:8] = 16w0";

    EXPECT_FALSE(program_string.find(value1) == std::string::npos);
    EXPECT_FALSE(program_string.find(value2) == std::string::npos);
    EXPECT_FALSE(program_string.find(value3) == std::string::npos);
}

}  // namespace Test
