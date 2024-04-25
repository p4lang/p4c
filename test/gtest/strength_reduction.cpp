#include <absl/strings/substitute.h>
#include <gtest/gtest.h>

#include <optional>

#include "frontends/p4/frontend.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/toP4/toP4.h"
#include "helpers.h"
#include "ir/dump.h"
#include "ir/ir.h"
#include "lib/sourceCodeBuilder.h"

using namespace P4;

namespace Test {

namespace {

std::optional<FrontendTestCase> createStrengthReductionTestCase(
    const std::string &ingressSource, P4::FrontEndPolicy *policy = nullptr) {
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
$0
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

    return FrontendTestCase::create(absl::Substitute(source, ingressSource),
                                    CompilerOptions::FrontendVersion::P4_16, policy);
}

}  // namespace

class StrengthReductionTest : public P4CTest {};

struct StrengthReductionPolicy : public P4::FrontEndPolicy {
    bool enableSubConstToAddTransform() const override { return enableSubConstToAddTransform_; }

    explicit StrengthReductionPolicy(bool enableSubConstToAddTransform = true)
        : enableSubConstToAddTransform_(enableSubConstToAddTransform) {}

 private:
    bool enableSubConstToAddTransform_ = true;
};

TEST_F(StrengthReductionTest, Default) {
    auto test = createStrengthReductionTestCase(P4_SOURCE(R"(headers.h.f1 = headers.h.f1 - 1;)"));

    ReferenceMap refMap;
    TypeMap typeMap;

    Util::SourceCodeBuilder builder;
    ToP4 top4(builder, false);
    test->program->apply(top4);

    std::string program_string = builder.toString();
    std::string value1 = "headers.h.f1 = headers.h.f1 + 32w4294967295";
    std::string value2 = "headers.h.f1 = headers.h.f1 - 32w1";
    EXPECT_FALSE(program_string.find(value1) == std::string::npos);
    EXPECT_TRUE(program_string.find(value2) == std::string::npos);
}

TEST_F(StrengthReductionTest, DisableSubConstToAddConst) {
    StrengthReductionPolicy policy(false);
    auto test =
        createStrengthReductionTestCase(P4_SOURCE(R"(headers.h.f1 = headers.h.f1 - 1;)"), &policy);

    ReferenceMap refMap;
    TypeMap typeMap;

    Util::SourceCodeBuilder builder;
    ToP4 top4(builder, false);
    test->program->apply(top4);

    std::string program_string = builder.toString();
    std::string value1 = "headers.h.f1 = headers.h.f1 + 32w4294967295";
    std::string value2 = "headers.h.f1 = headers.h.f1 - 32w1";
    EXPECT_TRUE(program_string.find(value1) == std::string::npos);
    EXPECT_FALSE(program_string.find(value2) == std::string::npos);
}

}  // namespace Test
