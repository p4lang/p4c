#include <boost/algorithm/string/trim_all.hpp>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "helpers.h"
#include "lib/sourceCodeBuilder.h"

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "midend/local_copyprop.h"

using namespace P4;

namespace Test {

class LocalCopyPropTest : public P4CTest { };

TEST_F(LocalCopyPropTest, DeadCodeRemoval) {
    std::string source = P4_SOURCE(P4Headers::V1MODEL, R"(
        extern Func {
            Func();
            abstract void apply(inout bit<32> value);
        }
        header H { bit<32> f1; bit<32> f2;}
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
            Func() func = {
                void apply(inout bit<32> val) {
                    val = headers.h.f1;
                    val = val + 1;
                }
            };
            apply {
                func.apply(headers.h.f2);
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
    auto test = FrontendTestCase::create(source, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    refMap.setIsV1(true);
    TypeMap typeMap;
    auto typeChecking = new TypeChecking(&refMap, &typeMap, true);

    Util::SourceCodeBuilder builder;
    auto dump = new ToP4(builder, false);

    PassManager quick_midend = {
        typeChecking,
        new LocalCopyPropagation(&refMap, &typeMap, typeChecking),
        dump
    };
    test->program->apply(quick_midend);

    std::string program_string = builder.toString();
    boost::algorithm::trim_fill(program_string, " ");
    // Check that implifyDefUse has removed dead code from:
    //    void apply(inout bit<32> val) { val = headers.h.f1; val = headers.h.f1 + 32w1; }";
    std::string expected = "void apply(inout bit<32> val) { ; val = headers.h.f1 + 32w1; }";
    EXPECT_TRUE(program_string.find(expected) != std::string::npos);
}

}  // namespace Test
